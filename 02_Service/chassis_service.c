/**
 * @file    chassis_service.c
 * @brief   底盘服务：命令队列 + 20ms 控制任务 + 导航仲裁
 * @author  haoyu
 * @note    - 任务每 20ms：读 hwt101 yaw（ERR_RES 沿用上次）→ chassis_odom →
 *            按命令 chassis_nav/follow/set_vel/set_radius → 上报
 *          - chassis_odom 内部自取电机脉冲，本层不碰电机、无需互斥
 *          - 命令队列深度 1，新命令覆盖；导航未完成时拒绝新导航(BUSY)
 */

#include "chassis_service.h"

#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"

#include "chassis_adaption.h"
#include "system_assembly.h"
#include "hwt101_adaption.h"
#include "map_adaption.h"

#define CSVC_PERIOD_MS    20U        /* 控制周期 ms */
#define CSVC_ODOM_DT_S    0.02f      /* 里程计积分周期 s（=20ms） */
#define CSVC_RAD2DEG      57.29578f  /* rad→deg */
#define CSVC_PATH_MAX     128U       /* 路径点缓存上限 */
#define CSVC_TASK_STACK   2048U      /* 控制任务栈字节 */

/* 内部命令模式 */
typedef enum {
    CMD_NONE = 0,  /* 无命令 */
    CMD_FREE,      /* 自由速度 */
    CMD_ARC,       /* 画圆 */
    CMD_LINE,      /* 直线导航 */
    CMD_PATH       /* 路径跟随 */
} csvc_cmd_mode_t;

/* 内部命令（投递到队列） */
typedef struct {
    csvc_cmd_mode_t mode;   /* 命令类型 */
    union {
        struct { float vx, vy, wz; } fr;              /* FREE，wz deg/s */
        struct { float linear, r; bool insitu; } arc; /* ARC */
        struct { map_point_t tgt; float yaw, v, w; uint8_t *fin; } line; /* LINE */
        struct { float yaw, v; uint16_t len; uint8_t *fin; } path;      /* PATH */
    } u;
} csvc_cmd_t;

static osMessageQueueId_t g_queue = NULL;    /* 命令队列（深度 1） */
static osThreadId_t       g_task = NULL;     /* 控制任务句柄 */
static uint8_t            g_inited = 0U;     /* 是否已初始化 */
static float              g_last_yaw = 0.0f; /* 上次有效 yaw，deg */
static uint8_t           *g_nav_fin = NULL;  /* 当前导航完成标志（仲裁用） */
static map_point_t        g_path[CSVC_PATH_MAX]; /* 路径缓存（世界系 mm） */

static const osThreadAttr_t g_task_attr = {
    .name       = "chassis_svc",
    .stack_size = CSVC_TASK_STACK,
    .priority   = osPriorityHigh,
};

static void csvc_report(const chassis_odom_t *odom);
static void csvc_cancel_nav(void);
static csvc_status_t csvc_post(const csvc_cmd_t *cmd);
static csvc_status_t csvc_plan(map_point_t target, uint16_t *out_len);
static void csvc_task(void *arg);

/**
 * @brief  上报里程计快照给上位机（printf 重定向到上报 UART）
 * @param  odom 里程计快照
 */
static void csvc_report(const chassis_odom_t *odom)
{
    /* 6 元组：x,y,yaw(deg),vx,vy,w(deg/s)，与上位机网页约定 */
    printf("%d,%d,%d,%d,%d,%d\r\n",
           (int)odom->x_mm, (int)odom->y_mm,
           (int)(odom->yaw_rad * CSVC_RAD2DEG),
           (int)odom->vx, (int)odom->vy,
           (int)(odom->w * CSVC_RAD2DEG));
}

/**
 * @brief  取消进行中的导航（手动速度打断时把旧导航标记结束）
 */
static void csvc_cancel_nav(void)
{
    if (g_nav_fin != NULL) {
        *g_nav_fin = 1U;   /* 标记旧导航结束（被打断） */
        g_nav_fin = NULL;
    }
}

/**
 * @brief  覆盖式投递命令（深度 1 队列，先清空再放最新）
 * @param  cmd 待投递命令
 * @retval CSVC_OK / CSVC_ERR_INIT / CSVC_ERR
 */
static csvc_status_t csvc_post(const csvc_cmd_t *cmd)
{
    csvc_cmd_t drain; /* 清空旧命令的临时承接 */

    if ((g_inited == 0U) || (g_queue == NULL)) {
        return CSVC_ERR_INIT;
    }
    while (osMessageQueueGet(g_queue, &drain, NULL, 0U) == osOK) {
        /* 丢弃未消费的旧命令，只保留最新 */
    }
    if (osMessageQueuePut(g_queue, cmd, 0U, 0U) != osOK) {
        return CSVC_ERR;
    }
    return CSVC_OK;
}

/**
 * @brief  以里程计当前位姿为起点规划到目标的 A* 路径，缓存到 g_path
 * @param  target  目标点（世界系 mm）
 * @param  out_len 输出路径点数
 * @retval CSVC_OK / CSVC_ERR / CSVC_ERR_PARAM / CSVC_ERR_INIT
 * @note   起点用里程计位姿，须与地图世界系同系（上电由 app 调 set_pose 对齐）
 */
static csvc_status_t csvc_plan(map_point_t target, uint16_t *out_len)
{
    chassis_odom_t odom;          /* 当前位姿，作规划起点 */
    map_status_t   ms = MAP_ERR;  /* 规划结果 */
    uint16_t       len = 0U;      /* 路径点数 */

    if (out_len == NULL) {
        return CSVC_ERR_PARAM;
    }
    if (chassis_get_odom(&odom) != CHASSIS_OK) {
        return CSVC_ERR_INIT;
    }
    ms = map_adp_find_path((int16_t)odom.x_mm, (int16_t)odom.y_mm,
                           target.x_mm, target.y_mm,
                           g_path, CSVC_PATH_MAX, &len);
    if (ms != MAP_OK) {
        return CSVC_ERR;
    }
    *out_len = len;
    return CSVC_OK;
}

/**
 * @brief  底盘服务周期任务：里程计 + 命令执行（20ms）
 * @param  arg 未用
 */
static void csvc_task(void *arg)
{
    csvc_cmd_t     cmd;                           /* 当前执行命令 */
    chassis_odom_t odom;                          /* 里程计快照 */
    float          gyro = 0.0f;                   /* 角速度（附带量） */
    float          yaw = 0.0f;                    /* 航向 deg */
    uint32_t       wake = osKernelGetTickCount(); /* 周期唤醒基准 */

    (void)arg;
    (void)memset(&cmd, 0, sizeof(cmd));
    (void)memset(&odom, 0, sizeof(odom));
    for (;;) {
        /* 1) 取最新命令；无新命令沿用上一条 */
        (void)osMessageQueueGet(g_queue, &cmd, NULL, 0U);

        /* 2) 读 yaw：无新帧沿用上次有效值 */
        if (hwt101_adp_read(&gyro, &yaw) != HWT101_OK) {
            yaw = g_last_yaw;
        } else {
            g_last_yaw = yaw;
        }

        /* 3) 里程计：chassis_odom 内部读脉冲 + 取负 + deg→rad */
        (void)chassis_odom(yaw, CSVC_ODOM_DT_S);
        (void)chassis_get_odom(&odom);

        /* 4) 先算控制并下发速度（实时优先） */
        switch (cmd.mode) {
            case CMD_FREE:
                (void)chassis_set_vel(cmd.u.fr.vx, cmd.u.fr.vy, cmd.u.fr.wz);
                break;
            case CMD_ARC:
                (void)chassis_set_radius(cmd.u.arc.linear, cmd.u.arc.r,
                                         cmd.u.arc.insitu);
                break;
            case CMD_LINE:
                (void)chassis_nav(cmd.u.line.tgt, cmd.u.line.yaw,
                                  cmd.u.line.v, cmd.u.line.w, cmd.u.line.fin);
                if ((cmd.u.line.fin != NULL) && (*cmd.u.line.fin != 0U)) {
                    g_nav_fin = NULL;
                    cmd.mode = CMD_NONE;
                }
                break;
            case CMD_PATH:
                (void)chassis_follow(g_path, cmd.u.path.len, cmd.u.path.v,
                                     cmd.u.path.yaw, cmd.u.path.fin);
                if ((cmd.u.path.fin != NULL) && (*cmd.u.path.fin != 0U)) {
                    g_nav_fin = NULL;
                    cmd.mode = CMD_NONE;
                }
                break;
            default:
                break;
        }

        /* 5) 下发后再上报，避免阻塞控制计算 */
        csvc_report(&odom);
        wake += CSVC_PERIOD_MS;
        (void)osDelayUntil(wake);
    }
}

csvc_status_t csvc_init(void)
{
    motor_handler_t *mh = NULL; /* 电机 handler 句柄 */

    if (g_inited != 0U) {
        return CSVC_OK;
    }
    mh = system_motor_handler();
    if (mh == NULL) {
        return CSVC_ERR_INIT;            /* 电机子系统未组装 */
    }
    if (chassis_init(mh) != CHASSIS_OK) {
        return CSVC_ERR_INIT;            /* 接入底盘失败 */
    }
    if (map_adp_init() != MAP_OK) {
        return CSVC_ERR_INIT;            /* 地图/障碍载入失败 */
    }
    g_queue = osMessageQueueNew(1U, sizeof(csvc_cmd_t), NULL);
    if (g_queue == NULL) {
        return CSVC_ERR_RES;
    }
    g_task = osThreadNew(csvc_task, NULL, &g_task_attr);
    if (g_task == NULL) {
        return CSVC_ERR_RES;
    }
    g_inited = 1U;
    return CSVC_OK;
}

csvc_status_t csvc_free(float vx, float vy, float wz)
{
    csvc_cmd_t cmd; /* 待投递命令 */

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.mode = CMD_FREE;
    cmd.u.fr.vx = vx;
    cmd.u.fr.vy = vy;
    cmd.u.fr.wz = wz;
    csvc_cancel_nav();              /* 手动速度打断进行中导航 */
    return csvc_post(&cmd);
}

csvc_status_t csvc_arc(float linear, float r, bool insitu)
{
    csvc_cmd_t cmd; /* 待投递命令 */

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.mode = CMD_ARC;
    cmd.u.arc.linear = linear;
    cmd.u.arc.r = r;
    cmd.u.arc.insitu = insitu;
    csvc_cancel_nav();              /* 手动画圆打断进行中导航 */
    return csvc_post(&cmd);
}

csvc_status_t csvc_nav(uint8_t mode, map_point_t target, float yaw_deg,
                       float v, float w, uint8_t *finished)
{
    csvc_cmd_t    cmd;                 /* 待投递命令 */
    csvc_status_t ret = CSVC_ERR;       /* 步骤结果 */
    uint16_t      len = 0U;            /* 路径点数 */

    if (finished == NULL) {
        return CSVC_ERR_PARAM;
    }
    if (g_inited == 0U) {
        return CSVC_ERR_INIT;
    }
    /* 仲裁：上一条导航未完成则拒绝新导航 */
    if ((g_nav_fin != NULL) && (*g_nav_fin == 0U)) {
        return CSVC_BUSY;
    }
    (void)memset(&cmd, 0, sizeof(cmd));

    if (mode == CSVC_NAV_LINE) {
        cmd.mode = CMD_LINE;
        cmd.u.line.tgt = target;
        cmd.u.line.yaw = yaw_deg;
        cmd.u.line.v = v;
        cmd.u.line.w = w;
        cmd.u.line.fin = finished;
    } else if (mode == CSVC_NAV_PATH) {
        ret = csvc_plan(target, &len);
        if (ret != CSVC_OK) {
            return ret;
        }
        cmd.mode = CMD_PATH;
        cmd.u.path.yaw = yaw_deg;
        cmd.u.path.v = v;
        cmd.u.path.len = len;
        cmd.u.path.fin = finished;
    } else {
        return CSVC_ERR_PARAM;
    }

    *finished = 0U;
    g_nav_fin = finished;
    ret = csvc_post(&cmd);
    if (ret != CSVC_OK) {
        *finished = 1U;
        g_nav_fin = NULL;
    }
    return ret;
}
