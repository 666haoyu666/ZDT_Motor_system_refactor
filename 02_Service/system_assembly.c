/**
 * @file    system_assembly.c
 * @brief   系统组装根：CMSIS-OS2 包装 + RX 蹦床 + 电机子系统装配
 * @author  haoyu
 * @note    - 这里是全工程唯一把 OS（CMSIS-OS2/FreeRTOS）注入 BSP handler 的地方
 *          - 装配顺序：adp_init → mh_inst → mh_start → adp_start → 使能 → 上报
 *          - 工程 tick = 1kHz，故 tick 数与 ms 一一对应
 */

#include "system_assembly.h"

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

#include "zdt_motor_adaption.h"
#include "hwt101_adaption.h"

/* ===== 调试日志：0=不编译进固件，1=经 RTT 输出 ===== */
#ifndef SYS_ASM_LOG_EN
#define SYS_ASM_LOG_EN   1
#endif
#if SYS_ASM_LOG_EN
#include "cat_log.h"
#define SYS_LOGI(fmt, ...)  LOGI("[sys] " fmt, ##__VA_ARGS__)
#define SYS_LOGE(fmt, ...)  LOGE("[sys] " fmt, ##__VA_ARGS__)
#else
#define SYS_LOGI(fmt, ...)  do {} while (0)
#define SYS_LOGE(fmt, ...)  do {} while (0)
#endif

#define SYS_MOTOR_REPORT_MS   2U     /* 电机位置上报周期 ms（里程计数据源） */
#define SYS_MH_STACK_BYTES    2048U  /* 收发线程栈：解析 + RTT 日志够用 */

/* ---- 被注入的对象 ---- */
static motor_handler_t g_motor_handler;   /* 电机总线 handler（一条命令总线）*/
static uint8_t         g_assembled = 0U;  /* 组装完成标志 */

/* ---- CMSIS-OS2：线程 ---- */

/* 收发线程共用属性（TX/RX 同优先级，解析负载轻） */
static const osThreadAttr_t g_mh_thread_attr = {
    .name       = "mh_worker",
    .stack_size = SYS_MH_STACK_BYTES,
    .priority   = osPriorityNormal,
};

/**
 * @brief  新建线程（注入给 handler）
 * @param  entry  线程体，签名 void(*)(void*)
 * @param  arg    传给线程体的参数
 * @param  handle 输出线程句柄
 * @retval ZDT_OK / ZDT_ERR_PARAM / ZDT_ERR_RES
 */
static zdt_status_t os_thread_new(void (*entry)(void *), void *arg,
                                  void **handle)
{
    osThreadId_t id = NULL; /* 新线程句柄 */

    if ((entry == NULL) || (handle == NULL)) {
        return ZDT_ERR_PARAM;
    }
    id = osThreadNew((osThreadFunc_t)entry, arg, &g_mh_thread_attr);
    if (id == NULL) {
        return ZDT_ERR_RES;
    }
    *handle = (void *)id;
    return ZDT_OK;
}

/* ---- CMSIS-OS2：消息队列 ---- */

/**
 * @brief  新建消息队列（注入给 handler）
 * @param  depth   队列深度
 * @param  item_sz 单条消息字节数
 * @param  handle  输出队列句柄
 * @retval ZDT_OK / ZDT_ERR_PARAM / ZDT_ERR_RES
 */
static zdt_status_t os_queue_new(uint32_t depth, uint32_t item_sz,
                                 void **handle)
{
    osMessageQueueId_t id = NULL; /* 新队列句柄 */

    if ((handle == NULL) || (depth == 0U) || (item_sz == 0U)) {
        return ZDT_ERR_PARAM;
    }
    id = osMessageQueueNew(depth, item_sz, NULL);
    if (id == NULL) {
        return ZDT_ERR_RES;
    }
    *handle = (void *)id;
    return ZDT_OK;
}

/**
 * @brief  入队一条消息（上层在 ISR / 任务均以 timeout=0 非阻塞投递）
 * @param  q          队列句柄
 * @param  item       消息源
 * @param  timeout_ms 超时 ms
 * @retval ZDT_OK / ZDT_ERR_PARAM / ZDT_ERR_RES
 */
static zdt_status_t os_queue_put(void *q, const void *item,
                                 uint32_t timeout_ms)
{
    if ((q == NULL) || (item == NULL)) {
        return ZDT_ERR_PARAM;
    }
    if (osMessageQueuePut((osMessageQueueId_t)q, item, 0U, timeout_ms) != osOK) {
        return ZDT_ERR_RES;
    }
    return ZDT_OK;
}

/**
 * @brief  出队一条消息
 * @param  q          队列句柄
 * @param  item       输出缓冲
 * @param  timeout_ms 超时 ms（MH_WAIT_FOREVER 即 osWaitForever）
 * @retval ZDT_OK / ZDT_ERR_PARAM / ZDT_ERR_RES（超时/空时线程据此重试）
 */
static zdt_status_t os_queue_get(void *q, void *item, uint32_t timeout_ms)
{
    if ((q == NULL) || (item == NULL)) {
        return ZDT_ERR_PARAM;
    }
    if (osMessageQueueGet((osMessageQueueId_t)q, item, NULL, timeout_ms) != osOK) {
        return ZDT_ERR_RES;
    }
    return ZDT_OK;
}

/* ---- CMSIS-OS2：时间 ---- */

/** @brief 阻塞延时 ms（tick=1kHz，ms 即 tick 数） */
static zdt_status_t os_delay_ms(uint32_t ms)
{
    (void)osDelay(ms);
    return ZDT_OK;
}

/** @brief 取系统 tick（帧间隔门控用） */
static uint32_t os_get_tick(void)
{
    return osKernelGetTickCount();
}

/* ---- 临界区：保护 pos_pulse 的 int64 跨线程读写 ---- */

/** @brief 进入临界区（关中断，护极短的位置读写） */
static void os_lock_enter(void)
{
    taskENTER_CRITICAL();
}

/** @brief 退出临界区 */
static void os_lock_exit(void)
{
    taskEXIT_CRITICAL();
}

/* ---- 注入接口实例 ---- */
static const mh_os_thread_t g_os_thread = { os_thread_new };
static const mh_os_queue_t  g_os_queue  = { os_queue_new, os_queue_put,
                                            os_queue_get };
static const mh_os_time_t   g_os_time   = { os_delay_ms, os_get_tick };
static const mh_os_lock_t   g_os_lock   = { os_lock_enter, os_lock_exit };

/**
 * @brief  RX 蹦床：适配层 ISR 回调签名 → handler 入队接口
 * @param  motor 来源电机
 * @param  data  帧缓冲
 * @param  len   帧长度
 * @note   ISR 上下文，仅做一次非阻塞入队
 */
static void motor_rx_trampoline(zdt_motor_t *motor, const uint8_t *data,
                                uint16_t len)
{
    (void)mh_feed_rx(&g_motor_handler, motor, data, len);
}

zdt_status_t system_assembly_init(void)
{
    zdt_status_t ret = ZDT_ERR; /* 步骤结果 */
    uint8_t i = 0U;            /* 电机下标 */

    /* 1) 平台适配：建电机/组 + 注入 HAL 收发，RX 回调指向蹦床 */
    ret = zdt_adp_init(motor_rx_trampoline);
    if (ret != ZDT_OK) {
        SYS_LOGE("adp_init fail=%d", (int)ret);
        return ret;
    }
    /* 2) 实例化 handler：绑定总线 + 注入 OS 接口 */
    ret = mh_inst(&g_motor_handler, zdt_adp_group(), &g_os_thread, &g_os_queue,
                 &g_os_time, &g_os_lock, MH_GAP_MS_MIN);
    if (ret != ZDT_OK) {
        SYS_LOGE("mh_inst fail=%d", (int)ret);
        return ret;
    }
    /* 3) 起 TX/RX 线程与队列 */
    ret = mh_start(&g_motor_handler);
    if (ret != ZDT_OK) {
        SYS_LOGE("mh_start fail=%d", (int)ret);
        return ret;
    }
    /* 4) 仅启动各路 RX（使能 / 上报作为 TX 经 handler 串行下发） */
    ret = zdt_adp_start();
    if (ret != ZDT_OK) {
        SYS_LOGE("adp_start fail=%d", (int)ret);
        return ret;
    }
    /* 5) 逐个使能板载电机 */
    for (i = 0U; i < ZDT_ADP_MOTOR_NUM; i++) {
        ret = mh_enable(&g_motor_handler, i, true);
        if (ret != ZDT_OK) {
            SYS_LOGE("mh_enable[%u] fail=%d", (unsigned)i, (int)ret);
            return ret;
        }
    }
    /* 6) 开启定时上报（里程计数据源） */
    ret = mh_report(&g_motor_handler, SYS_MOTOR_REPORT_MS);
    if (ret != ZDT_OK) {
        SYS_LOGE("mh_report fail=%d", (int)ret);
        return ret;
    }
    /* 7) 陀螺仪：装配 + 启动 DMA 接收（里程计 yaw 数据源） */
    if (hwt101_adp_init() != HWT101_OK) {
        SYS_LOGE("hwt101 init fail");
        return ZDT_ERR;
    }
    if (hwt101_adp_start() != HWT101_OK) {
        SYS_LOGE("hwt101 start fail");
        return ZDT_ERR;
    }
    g_assembled = 1U;
    SYS_LOGI("subsystems up: %u motors (report %ums) + hwt101",
             (unsigned)ZDT_ADP_MOTOR_NUM, (unsigned)SYS_MOTOR_REPORT_MS);
    return ZDT_OK;
}

motor_handler_t *system_motor_handler(void)
{
    if (g_assembled == 0U) {
        return NULL;
    }
    return &g_motor_handler;
}
