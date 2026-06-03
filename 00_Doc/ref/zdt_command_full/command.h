#ifndef ZDT_UART_COMMAND_H
#define ZDT_UART_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* 固定校验字节，当前命令层始终按照手册默认 0x6B 组帧。 */
#define ZDT_FIXED_CHECKSUM            ((uint8_t)0x6BU)

/* 多电机命令最小缓存长度：广播地址 + 功能码 + 总长度 + 校验码。 */
#define ZDT_MULTI_CMD_MIN_CAPACITY    ((uint16_t)5U)

/* 命令帧构建状态码。
 * 约定：OK 为 1，ERROR 为 0，便于兼容原有布尔式判断逻辑。 */
typedef enum
{
    ZDT_CMD_STATUS_ERROR     = 0x00U,
    ZDT_CMD_STATUS_OK        = 0x01U,
    ZDT_CMD_STATUS_PARAMETER = 0x02U,
    ZDT_CMD_STATUS_RESOURCE  = 0x03U,
    ZDT_CMD_STATUS_RESERVED  = 0x7FFFFFFFU
} zdt_cmd_status_t;
/* 电机方向：00 表示 CW，01 表示 CCW。
 * 手册里多个控制命令都复用这 1 字节定义。 */
typedef enum
{
    ZDT_DIRECTION_CW  = 0x00U,
    ZDT_DIRECTION_CCW = 0x01U
} ZDT_Direction;

/* 同步标志：00 立即执行，01 先缓存命令。
 * 当命令支持多机同步时，先发送 01，再用 5.3.14 广播触发。 */
typedef enum
{
    ZDT_SYNC_IMMEDIATE = 0x00U,
    ZDT_SYNC_BUFFERED  = 0x01U
} ZDT_SyncFlag;

/* 使能状态：00 不使能，01 使能。 */
typedef enum
{
    ZDT_ENABLE_DISABLE = 0x00U,
    ZDT_ENABLE_ENABLE  = 0x01U
} ZDT_EnableState;

/* 保存标志：00 不保存，01 保存到电机。 */
typedef enum
{
    ZDT_SAVE_NO  = 0x00U,
    ZDT_SAVE_YES = 0x01U
} ZDT_SaveFlag;

/* Emm 位置模式：相对上一目标、绝对位置、相对当前位置。 */
typedef enum
{
    ZDT_POSITION_RELATIVE_TO_LAST_TARGET = 0x00U,
    ZDT_POSITION_ABSOLUTE_TO_ZERO        = 0x01U,
    ZDT_POSITION_RELATIVE_TO_CURRENT     = 0x02U
} ZDT_PositionMode;

/* 回零模式。
 * 数值与手册 5.4.2 / 5.4.6 保持一致，直接作为原始协议值发送。 */
typedef enum
{
    ZDT_HOMING_SINGLE_NEAREST   = 0x00U,
    ZDT_HOMING_SINGLE_DIRECTION = 0x01U,
    ZDT_HOMING_COLLISION        = 0x02U,
    ZDT_HOMING_LIMIT_SWITCH     = 0x03U,
    ZDT_HOMING_ABSOLUTE_ZERO    = 0x04U,
    ZDT_HOMING_POWER_LOSS_POS   = 0x05U
} ZDT_HomingMode;

/* 控制模式：00 开环，01 闭环。 */
typedef enum
{
    ZDT_CONTROL_OPEN_LOOP   = 0x00U,
    ZDT_CONTROL_CLOSED_LOOP = 0x01U
} ZDT_ControlMode;

/* 固件类型。
 * 本命令层只实现 Emm 和通用命令，但仍保留协议原始枚举值，便于设置设备参数。 */
typedef enum
{
    ZDT_FIRMWARE_X        = 0x00U,
    ZDT_FIRMWARE_EMM      = 0x01U,
    ZDT_FIRMWARE_EMM_RAGE = 0x02U
} ZDT_FirmwareType;

/* 控制命令应答方式。 */
typedef enum
{
    ZDT_RESPONSE_NONE    = 0x00U,
    ZDT_RESPONSE_RECEIVE = 0x01U,
    ZDT_RESPONSE_REACHED = 0x02U,
    ZDT_RESPONSE_BOTH    = 0x03U,
    ZDT_RESPONSE_OTHER   = 0x04U
} ZDT_ResponseMode;

/* 设备侧通讯校验配置。
 * 注意：本命令层自身始终按固定 0x6B 发包，如果把设备改成其他校验方式，
 * 后续使用本命令层发送命令将不再匹配。 */
typedef enum
{
    ZDT_DEVICE_CHECKSUM_FIXED_6B   = 0x00U,
    ZDT_DEVICE_CHECKSUM_XOR        = 0x01U,
    ZDT_DEVICE_CHECKSUM_CRC8       = 0x02U,
    ZDT_DEVICE_CHECKSUM_MODBUS_RTU = 0x03U,
    ZDT_DEVICE_PROTOCOL_6B_DMX512  = 0x04U
} ZDT_DeviceChecksumMode;

/* 堵转保护模式。 */
typedef enum
{
    ZDT_STALL_PROTECTION_DISABLE     = 0x00U,
    ZDT_STALL_PROTECTION_ENABLE      = 0x01U,
    ZDT_STALL_PROTECTION_RETURN_ZERO = 0x02U
} ZDT_StallProtectionMode;

/* 脉冲端口复用模式。 */
typedef enum
{
    ZDT_PULSE_PORT_OFF     = 0x00U,
    ZDT_PULSE_PORT_OPEN    = 0x01U,
    ZDT_PULSE_PORT_FOC     = 0x02U,
    ZDT_PULSE_PORT_ESI_RCO = 0x03U,
    ZDT_PULSE_PORT_PLR_ESI = 0x04U
} ZDT_PulsePortMode;

/* 通讯端口复用模式。 */
typedef enum
{
    ZDT_COMM_PORT_OFF     = 0x00U,
    ZDT_COMM_PORT_ESI_ALO = 0x01U,
    ZDT_COMM_PORT_UART    = 0x02U,
    ZDT_COMM_PORT_CAN     = 0x03U,
    ZDT_COMM_PORT_ULR_ESI = 0x04U
} ZDT_CommPortMode;

/* En 引脚有效电平。 */
typedef enum
{
    ZDT_EN_ACTIVE_LOW  = 0x00U,
    ZDT_EN_ACTIVE_HIGH = 0x01U,
    ZDT_EN_ACTIVE_HOLD = 0x02U
} ZDT_EnActiveLevel;

/* 参数锁定等级。 */
typedef enum
{
    ZDT_PARAM_LOCK_UNLOCK = 0x00U,
    ZDT_PARAM_LOCK_COMM   = 0x01U,
    ZDT_PARAM_LOCK_ALL    = 0x02U,
    ZDT_PARAM_LOCK_STRICT = 0x03U
} ZDT_ParamLockLevel;

/* DMX512 运动模式：00 相对位置，01 绝对位置。 */
typedef enum
{
    ZDT_DMX_MOTION_RELATIVE = 0x00U,
    ZDT_DMX_MOTION_ABSOLUTE = 0x01U
} ZDT_DmxMotionMode;

/* 电机类型码。
 * 注意：手册不同小节对 0x19 / 0x32 的角度映射描述存在差异，
 * 这里直接保留原始协议值，由调用方按当前电机实际配置传入。 */
typedef enum
{
    ZDT_MOTOR_TYPE_CODE_19 = 0x19U,
    ZDT_MOTOR_TYPE_CODE_32 = 0x32U
} ZDT_MotorTypeCode;

/* 回零状态标志位定义，对应 5.4.4。 */
typedef enum
{
    ZDT_HOMING_STATUS_ENC_READY    = 0x01U,
    ZDT_HOMING_STATUS_CAL_READY    = 0x02U,
    ZDT_HOMING_STATUS_RUNNING      = 0x04U,
    ZDT_HOMING_STATUS_FAILED       = 0x08U,
    ZDT_HOMING_STATUS_OVER_TEMP    = 0x10U,
    ZDT_HOMING_STATUS_OVER_CURRENT = 0x20U
} ZDT_HomingStatusFlag;

/* 电机状态标志位定义，对应 5.5.15。 */
typedef enum
{
    ZDT_MOTOR_STATUS_ENABLED          = 0x01U,
    ZDT_MOTOR_STATUS_POSITION_REACHED = 0x02U,
    ZDT_MOTOR_STATUS_STALL_FLAG       = 0x04U,
    ZDT_MOTOR_STATUS_STALL_PROTECTED  = 0x08U,
    ZDT_MOTOR_STATUS_LEFT_LIMIT       = 0x10U,
    ZDT_MOTOR_STATUS_RIGHT_LIMIT      = 0x20U,
    ZDT_MOTOR_STATUS_POWER_LOSS       = 0x80U
} ZDT_MotorStatusFlag;

/* 引脚 IO 状态标志位定义，对应 5.5.17。 */
typedef enum
{
    ZDT_IO_STATE_EN_PIN     = 0x01U,
    ZDT_IO_STATE_STP_PIN    = 0x04U,
    ZDT_IO_STATE_DIR_PIN    = 0x10U,
    ZDT_IO_STATE_DIR_OUTPUT = 0x20U
} ZDT_IoStatusFlag;

/* Emm 速度模式参数。
 * 对应 5.3.7，字段顺序就是串口数据区顺序。 */
typedef struct
{
    ZDT_Direction direction;   /* 方向：00=CW，01=CCW。 */
    uint16_t speed_rpm;        /* 速度：0-30000 RPM。 */
    uint8_t acceleration;      /* 加速度档位：0-255。 */
    ZDT_SyncFlag sync_flag;    /* 同步标志：立即执行/先缓存。 */
} ZDT_EmmSpeedCmd;

/* Emm 位置模式参数。
 * 对应 5.3.12，脉冲数为原始协议值，不在本层做角度换算。 */
typedef struct
{
    ZDT_Direction direction;      /* 方向：00=CW，01=CCW。 */
    uint16_t speed_rpm;           /* 速度：0-30000 RPM。 */
    uint8_t acceleration;         /* 加速度档位：0-255。 */
    uint32_t pulse_count;         /* 脉冲数：0-0xFFFFFFFF。 */
    ZDT_PositionMode position_mode; /* 运动模式：00/01/02。 */
    ZDT_SyncFlag sync_flag;       /* 同步标志：立即执行/先缓存。 */
} ZDT_EmmPositionCmd;

/* 回零参数，对应 5.4.6。
 * 除 save_to_flash 外，其余字段均按手册原始单位传入。 */
typedef struct
{
    bool save_to_flash;             /* 是否保存：false=不保存，true=保存。 */
    uint8_t homing_mode;            /* 回零模式：0-5。 */
    ZDT_Direction homing_direction; /* 回零方向：00=CW，01=CCW。 */
    uint16_t homing_speed_rpm;      /* 回零速度：0-30000 RPM。 */
    uint32_t homing_timeout_ms;     /* 回零超时时间：毫秒。 */
    uint16_t collision_speed_rpm;   /* 碰撞检测转速：0-30000 RPM。 */
    uint16_t collision_current_ma;  /* 碰撞检测电流：0-5000 mA。 */
    uint16_t collision_time_ms;     /* 碰撞检测时间：0-65535 ms。 */
    bool enable_limit_input;        /* O_POT_En：false=关闭，true=开启。 */
} ZDT_HomingParams;

/* Emm PID 参数，对应 5.6.17。
 * 三个系数直接写入 4 字节无符号整数，不做比例缩放。 */
typedef struct
{
    bool save_to_flash;   /* 是否保存：false=不保存，true=保存。 */
    uint32_t kp;          /* 比例系数。 */
    uint32_t ki;          /* 积分系数。 */
    uint32_t kd;          /* 微分系数。 */
} ZDT_EmmPidParams;

/* DMX512 参数，对应 5.6.19。
 * 这里保留了手册所有字段，方便严格按协议一次性组完整数据区。 */
typedef struct
{
    bool save_to_flash;                /* 是否保存：false=不保存，true=保存。 */
    uint16_t total_channel_count;      /* 总通道数：1-64。 */
    uint8_t channels_per_motor;        /* 每个电机占用通道数：1 或 2。 */
    uint8_t motion_mode;               /* 运动模式：0=相对，1=绝对。 */
    uint16_t single_channel_speed_rpm; /* 单通道模式速度：1-30000 RPM。 */
    uint16_t acceleration;             /* 加速度参数：1-65535。 */
    uint16_t dual_channel_speed_step_rpm; /* 双通道速度步长：1-65535 RPM。 */
    uint32_t dual_channel_motion_step;    /* 双通道运动步长：1-0xFFFFFFFF。 */
} ZDT_Dmx512Params;

/* Emm 驱动配置，对应 5.8.6。
 * 该结构体字段较多，但顺序刻意与手册数据区顺序保持一致，便于核对。 */
typedef struct
{
    bool save_to_flash;                    /* 是否保存：false=不保存，true=保存。 */
    uint8_t motor_type_code;               /* 电机类型码：手册给出的有效值为 0x19/0x32。 */
    uint8_t pulse_port_mode;               /* 脉冲端口复用模式：0-4。 */
    uint8_t comm_port_mode;                /* 通讯端口复用模式：0-4。 */
    uint8_t en_pin_active_level;           /* En 引脚有效电平：0-2。 */
    uint8_t dir_pin_active_level;          /* Dir 引脚有效电平：0-1。 */
    uint8_t microstep;                     /* 细分：0x00=256，0x01-0xFF=1-255。 */
    bool microstep_interpolation;          /* 细分插补：false=关闭，true=开启。 */
    uint8_t reserved0;                     /* 保留字节，默认写 0。 */
    uint16_t open_loop_current_ma;         /* 开环模式工作电流：0-5000 mA。 */
    uint16_t closed_loop_stall_current_ma; /* 闭环模式堵转最大电流：0-5000 mA。 */
    uint16_t closed_loop_max_output_voltage; /* 闭环模式最大输出电压：0-5000。 */
    uint8_t uart_baud_index;               /* 串口波特率索引：0-8。 */
    uint8_t can_baud_index;                /* CAN 速率索引：0-9。 */
    uint8_t reserved1;                     /* 保留字节，默认写 0。 */
    uint8_t checksum_mode;                 /* 设备通讯协议/校验模式：0-4。 */
    uint8_t response_mode;                 /* 控制命令应答方式：0-4。 */
    uint8_t stall_protection_mode;         /* 堵转保护模式：0-2。 */
    uint16_t stall_detect_speed_rpm;       /* 堵转检测转速：0-30000 RPM。 */
    uint16_t stall_detect_current_ma;      /* 堵转检测电流：0-5000 mA。 */
    uint16_t stall_detect_time_ms;         /* 堵转检测时间：0-65535 ms。 */
    uint16_t position_window_0p1deg;       /* 位置到达窗口：单位 0.1°。 */
} ZDT_EmmDriveConfig;

/* Emm 上电自动运行参数，对应 5.7.2。
 * save_to_flash=false 时表示清除已存参数，而不是“发送但不保存”。 */
typedef struct
{
    bool save_to_flash;             /* false=清除已存储参数，true=存储当前参数。 */
    ZDT_Direction direction;        /* 方向：00=CW，01=CCW。 */
    uint16_t speed_rpm;             /* 速度：0-30000 RPM。 */
    uint8_t acceleration;           /* 加速度档位：0-255。 */
    bool enable_en_pin_control;     /* En 引脚控制启停：false=关闭，true=开启。 */
} ZDT_EmmAutoRunConfig;

/* 多电机命令 builder。
 * 典型流程：Init -> Append* -> Finalize/Send。 */
typedef struct
{
    uint8_t *buffer;     /* 外部提供的缓存。 */
    uint16_t capacity;   /* 缓存总长度。 */
    uint16_t length;     /* 当前已经写入的字节数。 */
    bool finalized;      /* 是否已经补齐总长度与末尾校验。 */
} ZDT_MultiCmdBuilder;

/* 5.3.1 多电机命令 builder：仅负责拼帧，不负责发送。 */
zdt_cmd_status_t ZDT_MultiCmdBuilderInit(ZDT_MultiCmdBuilder *builder,
                                         uint8_t *buffer,
                                         uint16_t capacity);
zdt_cmd_status_t ZDT_MultiCmdBuilderReset(ZDT_MultiCmdBuilder *builder);
zdt_cmd_status_t ZDT_MultiCmdBuilderAppendRaw(ZDT_MultiCmdBuilder *builder,
                                              const uint8_t *sub_frame,
                                              uint16_t sub_frame_size);
zdt_cmd_status_t ZDT_MultiCmdBuilderAppendRead(ZDT_MultiCmdBuilder *builder,
                                               uint8_t addr,
                                               uint8_t code);
zdt_cmd_status_t ZDT_MultiCmdBuilderAppendEnable(ZDT_MultiCmdBuilder *builder,
                                                 uint8_t addr,
                                                 ZDT_EnableState enable_state,
                                                 ZDT_SyncFlag sync_flag);
zdt_cmd_status_t ZDT_MultiCmdBuilderAppendEmmSpeed(ZDT_MultiCmdBuilder *builder,
                                                   uint8_t addr,
                                                   const ZDT_EmmSpeedCmd *cmd);
zdt_cmd_status_t ZDT_MultiCmdBuilderAppendEmmPosition(ZDT_MultiCmdBuilder *builder,
                                                      uint8_t addr,
                                                      const ZDT_EmmPositionCmd *cmd);
zdt_cmd_status_t ZDT_MultiCmdBuilderAppendStop(ZDT_MultiCmdBuilder *builder,
                                               uint8_t addr,
                                               ZDT_SyncFlag sync_flag);
zdt_cmd_status_t ZDT_MultiCmdBuilderFinalize(ZDT_MultiCmdBuilder *builder,
                                             uint16_t *out_size);

/* 单命令构帧接口：调用方传入帧数组指针和帧大小指针，输出完整有效帧。 */
/* 手册第5.2节，命令：校准编码器（0x06）。 */
zdt_cmd_status_t ZDT_BuildCalibrateEncoderFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.2节，命令：重启电机（0x08）。 */
zdt_cmd_status_t ZDT_BuildRebootMotorFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.2节，命令：清零当前位置（0x0A）。 */
zdt_cmd_status_t ZDT_BuildClearCurrentPositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.2节，命令：清除保护状态（0x0E）。 */
zdt_cmd_status_t ZDT_BuildClearProtectionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.2节，命令：恢复出厂设置（0x0F）。 */
zdt_cmd_status_t ZDT_BuildRestoreFactorySettingsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.3.2节，命令：电机使能控制（0xF3）。 */
zdt_cmd_status_t ZDT_BuildSetMotorEnableFrame(uint8_t addr, ZDT_EnableState enable_state, ZDT_SyncFlag sync_flag, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.3.7节，命令：Emm 速度模式控制（0xF6）。 */
zdt_cmd_status_t ZDT_BuildEmmRunSpeedFrame(uint8_t addr, const ZDT_EmmSpeedCmd *cmd, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.3.12节，命令：Emm 位置模式控制（0xFD）。 */
zdt_cmd_status_t ZDT_BuildEmmRunPositionFrame(uint8_t addr, const ZDT_EmmPositionCmd *cmd, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.3.13节，命令：停止电机（0xFE）。 */
zdt_cmd_status_t ZDT_BuildStopMotorFrame(uint8_t addr, ZDT_SyncFlag sync_flag, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.3.14节，命令：触发多机同步执行（0xFF）。 */
zdt_cmd_status_t ZDT_BuildTriggerMultiSyncFrame(uint8_t *frame, uint16_t *frame_size);
/* 手册第5.4.1节，命令：设置单圈回零零点（0x93）。 */
zdt_cmd_status_t ZDT_BuildSetSingleTurnHomeZeroFrame(uint8_t addr, bool save_to_flash, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.4.2节，命令：触发回零（0x9A）。 */
zdt_cmd_status_t ZDT_BuildStartHomingFrame(uint8_t addr, uint8_t homing_mode, ZDT_SyncFlag sync_flag, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.4.3节，命令：强制退出回零（0x9C）。 */
zdt_cmd_status_t ZDT_BuildAbortHomingFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.4.4节，命令：读取回零状态标志（0x3B）。 */
zdt_cmd_status_t ZDT_BuildRequestHomingStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.4.5节，命令：读取回零参数（0x22）。 */
zdt_cmd_status_t ZDT_BuildRequestHomingParamsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.4.6节，命令：修改回零参数（0x4C）。 */
zdt_cmd_status_t ZDT_BuildSetHomingParamsFrame(uint8_t addr, const ZDT_HomingParams *params, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5.1节，命令：定时返回信息（0x11）。 */
zdt_cmd_status_t ZDT_BuildSetPeriodicReportFrame(uint8_t addr, uint8_t info_code, uint16_t period_ms, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取版本信息（0x1F）。 */
zdt_cmd_status_t ZDT_BuildRequestVersionInfoFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取相电阻和相电感（0x20）。 */
zdt_cmd_status_t ZDT_BuildRequestPhaseResistanceInductanceFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取总线电压（0x24）。 */
zdt_cmd_status_t ZDT_BuildRequestBusVoltageFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取总线电流（0x26）。 */
zdt_cmd_status_t ZDT_BuildRequestBusCurrentFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取相电流（0x27）。 */
zdt_cmd_status_t ZDT_BuildRequestPhaseCurrentFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取编码器值（0x31）。 */
zdt_cmd_status_t ZDT_BuildRequestEncoderValueFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取输入脉冲数（0x32）。 */
zdt_cmd_status_t ZDT_BuildRequestInputPulseCountFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取目标位置（0x33）。 */
zdt_cmd_status_t ZDT_BuildRequestTargetPositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取实时目标位置（0x34）。 */
zdt_cmd_status_t ZDT_BuildRequestRealtimeTargetPositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取实时转速（0x35）。 */
zdt_cmd_status_t ZDT_BuildRequestRealtimeSpeedFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取驱动温度（0x39）。 */
zdt_cmd_status_t ZDT_BuildRequestDriverTemperatureFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取实时位置（0x36）。 */
zdt_cmd_status_t ZDT_BuildRequestRealtimePositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取位置误差（0x37）。 */
zdt_cmd_status_t ZDT_BuildRequestPositionErrorFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5.15节，命令：读取电机状态标志（0x3A）。 */
zdt_cmd_status_t ZDT_BuildRequestMotorStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5节，命令：读取回零状态和电机状态（0x3C）。 */
zdt_cmd_status_t ZDT_BuildRequestHomingAndMotorStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.5.17节，命令：读取IO状态（0x3D）。 */
zdt_cmd_status_t ZDT_BuildRequestIoStateFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.1节，命令：修改电机地址（0xAE）。 */
zdt_cmd_status_t ZDT_BuildSetMotorAddressFrame(uint8_t addr, bool save_to_flash, uint8_t new_addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.2节，命令：修改细分（0x84）。 */
zdt_cmd_status_t ZDT_BuildSetMicrostepFrame(uint8_t addr, bool save_to_flash, uint8_t microstep, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：修改掉电标志（0x50）。 */
zdt_cmd_status_t ZDT_BuildSetPowerLossFlagFrame(uint8_t addr, bool power_loss_flag, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取选项参数状态（0x1A）。 */
zdt_cmd_status_t ZDT_BuildRequestOptionStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.5节，命令：修改电机类型（0xD7）。 */
zdt_cmd_status_t ZDT_BuildSetMotorTypeFrame(uint8_t addr, bool save_to_flash, uint8_t motor_type_code, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.6节，命令：修改固件类型（0xD5）。 */
zdt_cmd_status_t ZDT_BuildSetFirmwareTypeFrame(uint8_t addr, bool save_to_flash, ZDT_FirmwareType firmware_type, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.7节，命令：修改控制模式（0x46）。 */
zdt_cmd_status_t ZDT_BuildSetControlModeFrame(uint8_t addr, bool save_to_flash, ZDT_ControlMode control_mode, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.8节，命令：修改电机方向（0xD4）。 */
zdt_cmd_status_t ZDT_BuildSetMotorDirectionFrame(uint8_t addr, bool save_to_flash, ZDT_Direction direction, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.9节，命令：修改按键锁定（0xD0）。 */
zdt_cmd_status_t ZDT_BuildSetButtonLockFrame(uint8_t addr, bool save_to_flash, bool lock_button, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.11节，命令：修改速度输入缩放（0x4F）。 */
zdt_cmd_status_t ZDT_BuildEmmSetSpeedInputScale10xFrame(uint8_t addr, bool save_to_flash, bool enable_scale, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.12节，命令：修改开环工作电流（0x44）。 */
zdt_cmd_status_t ZDT_BuildSetOpenLoopCurrentFrame(uint8_t addr, bool save_to_flash, uint16_t current_ma, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.13节，命令：修改闭环最大电流（0x45）。 */
zdt_cmd_status_t ZDT_BuildSetClosedLoopMaxCurrentFrame(uint8_t addr, bool save_to_flash, uint16_t current_ma, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取PID参数（0x21）。 */
zdt_cmd_status_t ZDT_BuildEmmRequestPidFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.17节，命令：修改PID参数（0x4A）。 */
zdt_cmd_status_t ZDT_BuildEmmSetPidFrame(uint8_t addr, const ZDT_EmmPidParams *params, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取DMX512参数（0x49）。 */
zdt_cmd_status_t ZDT_BuildRequestDmx512ParamsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.19节，命令：修改DMX512参数（0xD9）。 */
zdt_cmd_status_t ZDT_BuildSetDmx512ParamsFrame(uint8_t addr, const ZDT_Dmx512Params *params, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取位置到达窗口（0x41）。 */
zdt_cmd_status_t ZDT_BuildRequestPositionWindowFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.21节，命令：修改位置到达窗口（0xD1）。 */
zdt_cmd_status_t ZDT_BuildSetPositionWindowFrame(uint8_t addr, bool save_to_flash, uint16_t position_window_0p1deg, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取过热过流阈值（0x13）。 */
zdt_cmd_status_t ZDT_BuildRequestProtectionThresholdsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.23节，命令：修改过热过流阈值（0xD3）。 */
zdt_cmd_status_t ZDT_BuildSetProtectionThresholdsFrame(uint8_t addr, bool save_to_flash, uint16_t over_temp_celsius, uint16_t over_current_ma, uint16_t detect_time_ms, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取心跳保护时间（0x16）。 */
zdt_cmd_status_t ZDT_BuildRequestHeartbeatTimeFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.25节，命令：修改心跳保护时间（0x68）。 */
zdt_cmd_status_t ZDT_BuildSetHeartbeatTimeFrame(uint8_t addr, bool save_to_flash, uint32_t heartbeat_time_ms, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取积分限幅（0x23）。 */
zdt_cmd_status_t ZDT_BuildRequestIntegralLimitFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.27节，命令：修改积分限幅（0x4B）。 */
zdt_cmd_status_t ZDT_BuildSetIntegralLimitFrame(uint8_t addr, bool save_to_flash, uint32_t integral_limit, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6节，命令：读取碰撞回零返回角（0x3F）。 */
zdt_cmd_status_t ZDT_BuildRequestCollisionReturnAngleFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.29节，命令：修改碰撞回零返回角（0x5C）。 */
zdt_cmd_status_t ZDT_BuildSetCollisionReturnAngleFrame(uint8_t addr, bool save_to_flash, uint16_t angle_0p1deg, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.30节，命令：广播读取地址（0x15，广播地址0x00）。 */
zdt_cmd_status_t ZDT_BuildBroadcastRequestAddressFrame(uint8_t *frame, uint16_t *frame_size);
/* 手册第5.6.31节，命令：修改参数锁定等级（0xD6）。 */
zdt_cmd_status_t ZDT_BuildSetParameterLockLevelFrame(uint8_t addr, bool save_to_flash, uint8_t lock_level, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.7.2节，命令：Emm上电自动运行参数设置（0xF7）。 */
zdt_cmd_status_t ZDT_BuildEmmSetAutoRunFrame(uint8_t addr, const ZDT_EmmAutoRunConfig *config, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.8节，命令：读取全部系统状态参数（0x43）。 */
zdt_cmd_status_t ZDT_BuildEmmRequestAllSystemStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.8节，命令：读取驱动配置参数（0x42）。 */
zdt_cmd_status_t ZDT_BuildEmmRequestDriveConfigFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size);
/* 手册第5.8.6节，命令：修改驱动配置参数（0x48）。 */
zdt_cmd_status_t ZDT_BuildEmmSetDriveConfigFrame(uint8_t addr, const ZDT_EmmDriveConfig *config, uint8_t *frame, uint16_t *frame_size);

#ifdef __cplusplus
}
#endif

#endif /* ZDT_UART_COMMAND_H */



