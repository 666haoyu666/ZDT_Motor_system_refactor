/**
 * @file    app_main.c
 * @brief   应用入口：上电时序 + BLE 调度
 * @author  haoyu
 * @note    - 时序：system_assembly_init → csvc_init → chassis_set_pose → BLE
 *          - BLE：ISR(on_rx_raw)入队 → app_task 取队 ble_adp_process → on_nav
 *          - on_nav 给 rad，本层转 deg 再投 csvc（csvc 对上统一 deg）
 */

#include "app_main.h"

#include <string.h>

#include "cmsis_os2.h"

#include "system_assembly.h"
#include "chassis_service.h"
#include "chassis_adaption.h"
#include "ble_adaption.h"

/* ===== 调试日志：0=不编译进固件，1=经 RTT 输出 ===== */
#ifndef APP_LOG_EN
#define APP_LOG_EN   1
#endif
#if APP_LOG_EN
#include "cat_log.h"
#define APP_LOGI(fmt, ...)  LOGI("[app] " fmt, ##__VA_ARGS__)
#define APP_LOGE(fmt, ...)  LOGE("[app] " fmt, ##__VA_ARGS__)
#else
#define APP_LOGI(fmt, ...)  do {} while (0)
#define APP_LOGE(fmt, ...)  do {} while (0)
#endif

#define APP_RAD2DEG       57.29578f /* rad→deg */
#define APP_BLE_CHUNK     64U       /* 单次 BLE 字节块上限 */
#define APP_BLE_QDEPTH    8U        /* BLE 字节块队列深度 */
#define APP_TASK_STACK    2048U     /* BLE 调度任务栈字节 */

/* 上电初始位姿（世界系，按场地标定） */
#define APP_START_X_MM    1200
#define APP_START_Y_MM    350
#define APP_START_YAW_DEG 0.0f

/* BLE 原始字节块（ISR 拷入 → 任务取出喂解析器） */
typedef struct {
    uint8_t  data[APP_BLE_CHUNK]; /* 字节缓冲 */
    uint16_t len;                 /* 有效长度 */
} app_chunk_t;

static osMessageQueueId_t g_ble_q = NULL; /* BLE 字节块队列 */
static osThreadId_t       g_task = NULL;  /* BLE 调度任务 */
static uint8_t            g_nav_fin = 1U; /* 导航完成标志，1=空闲 */

static const osThreadAttr_t g_task_attr = {
    .name       = "app_ble",
    .stack_size = APP_TASK_STACK,
    .priority   = osPriorityNormal,
};

static void app_on_rx_raw(const uint8_t *data, uint16_t len);
static void app_on_nav(ble_nav_mode_t mode, float x_mm, float y_mm,
                       float yaw_rad, float v_mm_s, float w_rad_s);
static void app_task(void *arg);

/**
 * @brief  BLE 原始字节回调（ISR 上下文）：拷贝入队，非阻塞
 * @param  data 字节缓冲
 * @param  len  字节长度
 */
static void app_on_rx_raw(const uint8_t *data, uint16_t len)
{
    app_chunk_t chunk; /* 待入队字节块 */

    if ((data == NULL) || (len == 0U) || (g_ble_q == NULL)) {
        return;
    }
    chunk.len = (len > APP_BLE_CHUNK) ? APP_BLE_CHUNK : len;
    (void)memcpy(chunk.data, data, chunk.len);
    (void)osMessageQueuePut(g_ble_q, &chunk, 0U, 0U); /* ISR 非阻塞 */
}

/**
 * @brief  BLE 导航命令回调（任务上下文）：rad→deg 后投 csvc
 * @param  mode    导航模式
 * @param  x_mm    目标 x，mm
 * @param  y_mm    目标 y，mm
 * @param  yaw_rad 目标航向，rad
 * @param  v_mm_s  线速度，mm/s
 * @param  w_rad_s 角速度，rad/s
 */
static void app_on_nav(ble_nav_mode_t mode, float x_mm, float y_mm,
                       float yaw_rad, float v_mm_s, float w_rad_s)
{
    uint8_t     svc_mode; /* csvc 导航模式 */
    map_point_t tgt;      /* 目标点 */

    svc_mode = (mode == BLE_NAV_PATH) ? CSVC_NAV_PATH : CSVC_NAV_LINE;
    tgt.x_mm = (int16_t)x_mm;
    tgt.y_mm = (int16_t)y_mm;
    (void)csvc_nav(svc_mode, tgt, yaw_rad * APP_RAD2DEG, v_mm_s,
                   w_rad_s * APP_RAD2DEG, &g_nav_fin);
}

/**
 * @brief  BLE 调度任务：取出字节块喂解析器（阻塞等待）
 * @param  arg 未用
 */
static void app_task(void *arg)
{
    app_chunk_t chunk; /* 出队字节块 */

    (void)arg;
    for (;;) {
        if (osMessageQueueGet(g_ble_q, &chunk, NULL, osWaitForever) == osOK) {
            (void)ble_adp_process(chunk.data, chunk.len);
        }
    }
}

app_status_t app_init(void)
{
    map_point_t start; /* 上电初始位姿坐标 */

    /* 1) 装配电机 + 陀螺仪子系统 */
    if (system_assembly_init() != ZDT_OK) {
        APP_LOGE("assembly init fail");
        return APP_ERR;
    }
    /* 2) 起底盘服务（接入 chassis + map + 20ms 控制任务） */
    if (csvc_init() != CSVC_OK) {
        APP_LOGE("chassis service init fail");
        return APP_ERR;
    }
    /* 3) 里程计对齐世界系初始位姿 */
    start.x_mm = (int16_t)APP_START_X_MM;
    start.y_mm = (int16_t)APP_START_Y_MM;
    (void)chassis_set_pose(start, APP_START_YAW_DEG);
    /* 4) BLE 字节队列 + 适配层（注入 ISR 入队 / 命令上抛回调） */
    g_ble_q = osMessageQueueNew(APP_BLE_QDEPTH, sizeof(app_chunk_t), NULL);
    if (g_ble_q == NULL) {
        APP_LOGE("ble queue fail");
        return APP_ERR;
    }
    if (ble_adp_init(app_on_rx_raw, app_on_nav) != BLE_OK) {
        APP_LOGE("ble adp init fail");
        return APP_ERR;
    }
    /* 5) 起 BLE 调度任务 */
    g_task = osThreadNew(app_task, NULL, &g_task_attr);
    if (g_task == NULL) {
        APP_LOGE("app task fail");
        return APP_ERR;
    }
    APP_LOGI("app up");
    return APP_OK;
}
