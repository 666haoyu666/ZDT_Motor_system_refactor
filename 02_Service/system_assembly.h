/**
 * @file    system_assembly.h
 * @brief   系统组装根：建对象、注入 OS 接口、按序启动电机子系统
 * @author  haoyu
 * @note    - composition root：唯一允许见到各模块具体类型的地方
 *          - 由 freertos.c 的 MX_FREERTOS_Init 调一次
 *          - 对外只给「初始化入口 + 电机 handler 句柄」
 */

#ifndef SYSTEM_ASSEMBLY_H
#define SYSTEM_ASSEMBLY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zdt_motor_handler.h"   /* motor_handler_t / zdt_status_t */

/**
 * @brief  组装并启动电机子系统（适配→handler→RX→使能→上报）
 * @retval ZDT_OK / 透传首个失败步骤的状态码
 */
zdt_status_t system_assembly_init(void);

/**
 * @brief  取电机总线 handler 句柄（供底盘 / 业务下发命令）
 * @retval handler 指针；未组装完成返回 NULL
 */
motor_handler_t *system_motor_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_ASSEMBLY_H */
