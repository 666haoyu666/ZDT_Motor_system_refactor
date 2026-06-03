#include "command.h"

#include <string.h>

/* 当前命令层只按固定 0x6B 方式组帧。 */
#define ZDT_FRAME_MAX_SIZE  ((uint16_t)40U)

typedef struct
{
    uint8_t *p_frame;
    uint16_t *p_frame_size;
    uint8_t is_active;
} zdt_build_context_t;

static zdt_build_context_t s_zdt_build_context = { NULL, NULL, 0U };

static bool zdt_begin_capture_frame(uint8_t *frame, uint16_t *frame_size)
{
    if ((frame == NULL) || (frame_size == NULL))
    {
        return false;
    }

    s_zdt_build_context.p_frame = frame;
    s_zdt_build_context.p_frame_size = frame_size;
    s_zdt_build_context.is_active = 1U;
    *frame_size = 0U;
    return true;
}

static void zdt_end_capture_frame(void)
{
    s_zdt_build_context.p_frame = NULL;
    s_zdt_build_context.p_frame_size = NULL;
    s_zdt_build_context.is_active = 0U;
}

static bool zdt_capture_frame_data(const uint8_t *frame, uint16_t frame_size)
{
    if ((frame == NULL) || (frame_size == 0U))
    {
        return false;
    }

    if ((s_zdt_build_context.is_active == 0U) ||
        (s_zdt_build_context.p_frame == NULL) ||
        (s_zdt_build_context.p_frame_size == NULL))
    {
        return false;
    }

    (void)memcpy(s_zdt_build_context.p_frame, frame, frame_size);
    *(s_zdt_build_context.p_frame_size) = frame_size;
    return true;
}

/* 功能码定义。
 * 这里直接保留手册中的原始功能码，方便和第 5 章表格逐项对照。 */
enum
{
    ZDT_CODE_CALIBRATE_ENCODER         = 0x06U, /* 触发编码器校准            */
    ZDT_CODE_REBOOT_MOTOR              = 0x08U, /* 重启电机                 */
    ZDT_CODE_CLEAR_CURRENT_POSITION    = 0x0AU, /* 将当前位置清零           */
    ZDT_CODE_CLEAR_PROTECTION          = 0x0EU, /* 清除堵转/过热/过流保护    */
    ZDT_CODE_RESTORE_FACTORY           = 0x0FU, /* 恢复出厂设置             */
    ZDT_CODE_PERIODIC_REPORT           = 0x11U, /* 定时返回信息             */
    ZDT_CODE_READ_OVER_TEMP_CURRENT    = 0x13U, /* 读取过热过流保护阈值     */
    ZDT_CODE_READ_BROADCAST_ADDRESS    = 0x15U, /* 广播读取地址             */
    ZDT_CODE_READ_HEARTBEAT_TIME       = 0x16U, /* 读取心跳保护时间         */
    ZDT_CODE_READ_OPTION_STATUS        = 0x1AU, /* 读取选项参数状态         */
    ZDT_CODE_READ_VERSION              = 0x1FU, /* 读取固件/硬件版本        */
    ZDT_CODE_READ_PHASE_RL             = 0x20U, /* 读取相电阻和相电感       */
    ZDT_CODE_READ_PID                  = 0x21U, /* 读取 PID 参数            */
    ZDT_CODE_READ_HOMING_PARAMS        = 0x22U, /* 读取回零参数             */
    ZDT_CODE_READ_INTEGRAL_LIMIT       = 0x23U, /* 读取积分限幅/刚性系数    */
    ZDT_CODE_READ_BUS_VOLTAGE          = 0x24U, /* 读取总线电压             */
    ZDT_CODE_READ_BUS_CURRENT          = 0x26U, /* 读取总线电流             */
    ZDT_CODE_READ_PHASE_CURRENT        = 0x27U, /* 读取相电流               */
    ZDT_CODE_READ_ENCODER_VALUE        = 0x31U, /* 读取编码器值             */
    ZDT_CODE_READ_INPUT_PULSE          = 0x32U, /* 读取输入脉冲数           */
    ZDT_CODE_READ_TARGET_POSITION      = 0x33U, /* 读取目标位置             */
    ZDT_CODE_READ_REALTIME_TARGET      = 0x34U, /* 读取实时目标位置         */
    ZDT_CODE_READ_REALTIME_SPEED       = 0x35U, /* 读取实时转速             */
    ZDT_CODE_READ_REALTIME_POSITION    = 0x36U, /* 读取实时位置             */
    ZDT_CODE_READ_POSITION_ERROR       = 0x37U, /* 读取位置误差             */
    ZDT_CODE_READ_DRIVER_TEMPERATURE   = 0x39U, /* 读取驱动温度             */
    ZDT_CODE_READ_MOTOR_STATUS         = 0x3AU, /* 读取电机状态标志         */
    ZDT_CODE_READ_HOMING_STATUS        = 0x3BU, /* 读取回零状态标志         */
    ZDT_CODE_READ_HOMING_MOTOR_STATUS  = 0x3CU, /* 读取回零状态和电机状态   */
    ZDT_CODE_READ_IO_STATE             = 0x3DU, /* 读取 IO 电平状态         */
    ZDT_CODE_READ_COLLISION_RETURN     = 0x3FU, /* 读取碰撞回零返回角度     */
    ZDT_CODE_READ_POSITION_WINDOW      = 0x41U, /* 读取位置到达窗口         */
    ZDT_CODE_READ_DRIVE_CONFIG         = 0x42U, /* 读取驱动配置参数         */
    ZDT_CODE_READ_ALL_SYSTEM_STATUS    = 0x43U, /* 读取系统状态参数         */
    ZDT_CODE_SET_OPEN_LOOP_CURRENT     = 0x44U, /* 修改开环模式工作电流     */
    ZDT_CODE_SET_CLOSED_LOOP_CURRENT   = 0x45U, /* 修改闭环模式最大电流     */
    ZDT_CODE_SET_CONTROL_MODE          = 0x46U, /* 修改控制模式             */
    ZDT_CODE_SET_DRIVE_CONFIG          = 0x48U, /* 修改驱动配置参数         */
    ZDT_CODE_READ_DMX512_PARAMS        = 0x49U, /* 读取 DMX512 参数         */
    ZDT_CODE_SET_PID                   = 0x4AU, /* 修改 PID 参数            */
    ZDT_CODE_SET_INTEGRAL_LIMIT        = 0x4BU, /* 修改积分限幅/刚性系数    */
    ZDT_CODE_SET_HOMING_PARAMS         = 0x4CU, /* 修改回零参数             */
    ZDT_CODE_SET_SPEED_SCALE_10X       = 0x4FU, /* 修改速度输入缩放         */
    ZDT_CODE_SET_POWER_LOSS_FLAG       = 0x50U, /* 修改掉电标志             */
    ZDT_CODE_SET_COLLISION_RETURN      = 0x5CU, /* 修改碰撞回零返回角度     */
    ZDT_CODE_SET_HEARTBEAT_TIME        = 0x68U, /* 修改心跳保护时间         */
    ZDT_CODE_SET_MICROSTEP             = 0x84U, /* 修改细分值               */
    ZDT_CODE_SET_SINGLE_TURN_ZERO      = 0x93U, /* 设置单圈回零零点         */
    ZDT_CODE_START_HOMING              = 0x9AU, /* 触发回零                 */
    ZDT_CODE_ABORT_HOMING              = 0x9CU, /* 强制退出回零             */
    ZDT_CODE_SET_MOTOR_ADDRESS         = 0xAEU, /* 修改电机地址             */
    ZDT_CODE_SET_POSITION_WINDOW       = 0xD1U, /* 修改位置到达窗口         */
    ZDT_CODE_SET_PROTECTION_THRESHOLDS = 0xD3U, /* 修改过热过流保护阈值     */
    ZDT_CODE_SET_MOTOR_DIRECTION       = 0xD4U, /* 修改电机正方向           */
    ZDT_CODE_SET_FIRMWARE_TYPE         = 0xD5U, /* 修改固件类型             */
    ZDT_CODE_SET_PARAM_LOCK_LEVEL      = 0xD6U, /* 修改参数锁定等级         */
    ZDT_CODE_SET_MOTOR_TYPE            = 0xD7U, /* 修改电机类型             */
    ZDT_CODE_SET_DMX512_PARAMS         = 0xD9U, /* 修改 DMX512 参数         */
    ZDT_CODE_SET_BUTTON_LOCK           = 0xD0U, /* 修改按键锁定功能         */
    ZDT_CODE_MOTOR_ENABLE              = 0xF3U, /* 电机使能控制             */
    ZDT_CODE_EMM_SPEED                 = 0xF6U, /* Emm 速度模式控制         */
    ZDT_CODE_EMM_AUTO_RUN              = 0xF7U, /* Emm 上电自动运行         */
    ZDT_CODE_EMM_POSITION              = 0xFDU, /* Emm 位置模式控制         */
    ZDT_CODE_STOP_MOTOR                = 0xFEU, /* 立即停止                 */
    ZDT_CODE_TRIGGER_MULTI_SYNC        = 0xFFU, /* 触发多机同步             */
    ZDT_CODE_MULTI_MOTOR               = 0xAAU  /* 多电机命令               */
};

/* 辅助码定义。
 * 只有部分命令除了功能码外还要求携带一个辅助字节，
 * 这里同样严格按手册给出的固定值保存。 */
enum
{
    ZDT_ASSIST_CALIBRATE_ENCODER         = 0x45U, /* 编码器校准辅助码         */
    ZDT_ASSIST_REBOOT_MOTOR              = 0x97U, /* 电机重启辅助码           */
    ZDT_ASSIST_CLEAR_CURRENT_POSITION    = 0x6DU, /* 当前位置清零辅助码       */
    ZDT_ASSIST_CLEAR_PROTECTION          = 0x52U, /* 清除保护辅助码           */
    ZDT_ASSIST_RESTORE_FACTORY           = 0x5FU, /* 恢复出厂辅助码           */
    ZDT_ASSIST_PERIODIC_REPORT           = 0x18U, /* 定时返回信息辅助码       */
    ZDT_ASSIST_SET_SINGLE_TURN_ZERO      = 0x88U, /* 设置单圈零点辅助码       */
    ZDT_ASSIST_ENABLE                    = 0xABU, /* 电机使能辅助码           */
    ZDT_ASSIST_SET_HOMING_PARAMS         = 0xAEU, /* 修改回零参数辅助码       */
    ZDT_ASSIST_ABORT_HOMING              = 0x48U, /* 中断回零辅助码           */
    ZDT_ASSIST_READ_ALL_SYSTEM_STATUS    = 0x7AU, /* 读取系统状态辅助码       */
    ZDT_ASSIST_READ_DRIVE_CONFIG         = 0x6CU, /* 读取驱动配置辅助码       */
    ZDT_ASSIST_SET_DRIVE_CONFIG          = 0xD1U, /* 修改驱动配置辅助码       */
    ZDT_ASSIST_SET_PID                   = 0xC3U, /* 修改 PID 辅助码          */
    ZDT_ASSIST_READ_DMX512_PARAMS        = 0x78U, /* 读取 DMX512 辅助码       */
    ZDT_ASSIST_SET_DMX512_PARAMS         = 0x90U, /* 修改 DMX512 辅助码       */
    ZDT_ASSIST_SET_OPEN_LOOP_CURRENT     = 0x33U, /* 开环电流设置辅助码       */
    ZDT_ASSIST_SET_CLOSED_LOOP_CURRENT   = 0x66U, /* 闭环电流设置辅助码       */
    ZDT_ASSIST_SET_CONTROL_MODE          = 0xA6U, /* 控制模式设置辅助码       */
    ZDT_ASSIST_SET_SPEED_SCALE_10X       = 0x71U, /* 速度缩放设置辅助码       */
    ZDT_ASSIST_SET_MOTOR_ADDRESS         = 0x4BU, /* 电机地址设置辅助码       */
    ZDT_ASSIST_SET_MOTOR_DIRECTION       = 0x60U, /* 电机方向设置辅助码       */
    ZDT_ASSIST_SET_FIRMWARE_TYPE         = 0x69U, /* 固件类型设置辅助码       */
    ZDT_ASSIST_SET_MOTOR_TYPE            = 0x35U, /* 电机类型设置辅助码       */
    ZDT_ASSIST_SET_BUTTON_LOCK           = 0xB3U, /* 按键锁定设置辅助码       */
    ZDT_ASSIST_SET_POSITION_WINDOW       = 0x07U, /* 位置窗口设置辅助码       */
    ZDT_ASSIST_SET_PROTECTION_THRESHOLDS = 0x56U, /* 保护阈值设置辅助码       */
    ZDT_ASSIST_SET_HEARTBEAT_TIME        = 0x38U, /* 心跳时间设置辅助码       */
    ZDT_ASSIST_SET_INTEGRAL_LIMIT        = 0x57U, /* 积分限幅设置辅助码       */
    ZDT_ASSIST_SET_COLLISION_RETURN      = 0xACU, /* 碰撞返回角设置辅助码     */
    ZDT_ASSIST_SET_PARAM_LOCK_LEVEL      = 0x4BU, /* 参数锁定设置辅助码       */
    ZDT_ASSIST_EMM_AUTO_RUN              = 0x1CU, /* 上电自动运行辅助码       */
    ZDT_ASSIST_TRIGGER_MULTI_SYNC        = 0x66U  /* 多机同步触发辅助码       */
};

/* 以下辅助校验函数只负责做最基本的本地参数兜底，
 * 目的是避免明显越界的数据被发到电机侧。 */
/* 检查地址是否为单机地址。
 * 这里要求地址不能为 0，广播命令是否允许 0 由上层单独控制。 */
static bool zdt_is_valid_addr(uint8_t addr)
{
    return (addr != 0U);
}

/* 检查 0/1 两态标志位是否有效。
 * 方向、同步、保存、开关量等很多字段都复用这个判断。 */
static bool zdt_is_flag(uint8_t value)
{
    return (value <= 1U);
}

/* 检查 Emm 模式下的速度是否在手册范围内。
 * 当前项目已把速度单位扩展为十倍，所以上限是 30000 RPM。 */
static bool zdt_is_emm_speed(uint16_t speed_rpm)
{
    return (speed_rpm <= 30000U);
}

/* 检查电流参数是否在手册范围内。
 * 0x1388 = 5000 mA。 */
static bool zdt_is_current_ma(uint16_t current_ma)
{
    return (current_ma <= 0x1388U);
}

/* 检查回零模式编号是否有效。
 * 当前支持 0-5，对应手册里的 6 种回零方式。 */
static bool zdt_is_homing_mode(uint8_t homing_mode)
{
    return (homing_mode <= 0x05U);
}

/* 检查 Emm 位置模式是否有效。
 * 允许 0/1/2，分别对应三种位置模式。 */
static bool zdt_is_position_mode(uint8_t position_mode)
{
    return (position_mode <= 0x02U);
}

/* 检查串口波特率索引是否有效。
 * 手册给出的枚举范围是 0-8。 */
static bool zdt_is_uart_baud_index(uint8_t value)
{
    return (value <= 0x08U);
}

/* 检查 CAN 波特率索引是否有效。
 * 手册给出的枚举范围是 0-9。 */
static bool zdt_is_can_baud_index(uint8_t value)
{
    return (value <= 0x09U);
}

/* 检查设备侧通讯校验模式枚举是否有效。
 * 虽然本命令层固定发 0x6B，但仍允许写入设备配置。 */
static bool zdt_is_checksum_mode(uint8_t value)
{
    return (value <= 0x04U);
}

/* 检查控制命令应答方式是否有效。
 * 手册枚举范围为 0-4。 */
static bool zdt_is_response_mode(uint8_t value)
{
    return (value <= 0x04U);
}

/* 检查堵转保护模式是否有效。
 * 允许关闭、保护、保护后回零三种模式。 */
static bool zdt_is_stall_protection_mode(uint8_t value)
{
    return (value <= 0x02U);
}

/* 检查端口复用模式是否有效。
 * 脉冲端口和通讯端口都共用 0-4 这一组枚举范围。 */
static bool zdt_is_port_mode(uint8_t value)
{
    return (value <= 0x04U);
}

/* 检查 En 引脚有效电平配置是否有效。
 * 手册允许低有效、高有效、保持使能三种取值。 */
static bool zdt_is_en_active_level(uint8_t value)
{
    return (value <= 0x02U);
}

/* 检查电机类型码是否为手册列出的有效原始值。
 * 目前只接受 0x19 也就是1.8度电机 */
static bool zdt_is_motor_type_code(uint8_t value)
{
    return value == ((uint8_t)ZDT_MOTOR_TYPE_CODE_19);
}

/* 检查参数锁定等级是否有效。
 * 手册枚举范围为 0-3。 */
static bool zdt_is_param_lock_level(uint8_t value)
{
    return (value <= 0x03U);
}

/* 检查 DMX512 总通道数是否有效。
 * 手册要求总通道数范围为 1-64, 只有485类型电机才有效，可以省略 */
static bool zdt_is_dmx_channel_count(uint16_t value)
{
    return ((value >= 0x0001U) && (value <= 0x0040U));
}

/* 检查每个电机占用的 DMX 通道数是否有效。
 * 手册只允许 1 通道或 2 通道, 上同 */
static bool zdt_is_dmx_channels_per_motor(uint8_t value)
{
    return ((value == 0x01U) || (value == 0x02U));
}

/* 检查 DMX 运动模式是否有效。
 * 0 表示相对位置，1 表示绝对位置, 上同 */
static bool zdt_is_dmx_motion_mode(uint8_t value)
{
    return (value <= 0x01U);
}

/* 检查定时主动上报命令里填写的信息功能码是否允许。
 * 这里只接受手册 5.5.1 列出来那几种可周期返回的数据。 */
static bool zdt_is_periodic_report_code(uint8_t code)
{
    switch (code)
    {
        case ZDT_CODE_READ_VERSION:
        case ZDT_CODE_READ_PHASE_RL:
        case ZDT_CODE_READ_BUS_VOLTAGE:
        case ZDT_CODE_READ_BUS_CURRENT:
        case ZDT_CODE_READ_PHASE_CURRENT:
        case ZDT_CODE_READ_ENCODER_VALUE:
        case ZDT_CODE_READ_INPUT_PULSE:
        case ZDT_CODE_READ_TARGET_POSITION:
        case ZDT_CODE_READ_REALTIME_TARGET:
        case ZDT_CODE_READ_REALTIME_SPEED:
        case ZDT_CODE_READ_DRIVER_TEMPERATURE:
        case ZDT_CODE_READ_REALTIME_POSITION:
        case ZDT_CODE_READ_POSITION_ERROR:
        case ZDT_CODE_READ_MOTOR_STATUS:
        case ZDT_CODE_READ_HOMING_MOTOR_STATUS:
        case ZDT_CODE_READ_IO_STATE:
            return true;
        default:
            return false;
    }
}

/* 手册示例中的多字节字段均按高字节在前发送，因此统一封装成大端写入。 */
static void zdt_write_u16_be(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value >> 8);
    buffer[1] = (uint8_t)(value & 0xFFU);
}

static void zdt_write_u32_be(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value >> 24);
    buffer[1] = (uint8_t)((value >> 16) & 0xFFU);
    buffer[2] = (uint8_t)((value >> 8) & 0xFFU);
    buffer[3] = (uint8_t)(value & 0xFFU);
}

static bool zdt_build_frame(uint8_t addr,           //电机地址
                            bool allow_broadcast,   //是否允许广播
                            uint8_t code,           //功能码
                            const uint8_t *payload, //数据内容数组
                            uint16_t payload_len,   //数据内容长度
                            uint8_t *frame,         //全帧数据地址，设成指针也方便后续dma发送
                            uint16_t frame_capacity,//全帧最长长度，根据命令类型确定
                            uint16_t *frame_size)   //全帧大小
{
    uint16_t total_size;

    /* frame/frame_size 由调用方提供，用于输出最终串口帧和长度。 */
    if ((frame == NULL) || (frame_size == NULL))
    {
        return false;
    }

    /* 广播地址固定为 0。
     * 只有手册明确说明支持广播的命令，才允许在这里传入地址 0。 */
    if ((addr == 0U) && (!allow_broadcast))
    {
        return false;
    }

    total_size = (uint16_t)(payload_len + 3U);
    if (total_size > frame_capacity)
    {
        return false;
    }

    /* 串口帧固定格式：地址 + 功能码 + 数据区 + 0x6B。 */
    frame[0] = addr;
    frame[1] = code;
    if ((payload != NULL) && (payload_len > 0U))
    {
        (void)memcpy(&frame[2], payload, payload_len);
    }
    frame[2U + payload_len] = ZDT_FIXED_CHECKSUM;
    *frame_size = total_size;
    return true;
}

static bool zdt_send_frame(uint8_t addr,           //电机地址
                           bool allow_broadcast,   //是否允许广播地址 0
                           uint8_t code,           //功能码
                           const uint8_t *payload, //数据区首地址
                           uint16_t payload_len)   //数据区长度
{
    uint8_t frame[ZDT_FRAME_MAX_SIZE];
    uint16_t frame_size;

    /* 统一先组完整帧，再通过唯一发送出口 ZDT_send 发出。 */
    if (!zdt_build_frame(addr,
                         allow_broadcast,
                         code,
                         payload,
                         payload_len,
                         frame,
                         (uint16_t)sizeof(frame),
                         &frame_size))
    {
        return false;
    }

    return zdt_capture_frame_data(frame, frame_size);
}

/* 简单读取命令统一是“地址 + 功能码 + 0x6B”，因此用宏减少重复代码。 */
#define ZDT_DEFINE_SIMPLE_READ(func_name, code_value) \
    bool func_name(uint8_t addr) \
    { \
        return zdt_send_frame(addr, false, (uint8_t)(code_value), NULL, 0U); \
    }

/* 触发类命令通常是“地址 + 功能码 + 辅助码 + 0x6B”，同样用宏统一展开。 */
#define ZDT_DEFINE_SIMPLE_ACTION(func_name, code_value, assist_value) \
    bool func_name(uint8_t addr) \
    { \
        const uint8_t payload[] = { (uint8_t)(assist_value) }; \
        return zdt_send_frame(addr, false, (uint8_t)(code_value), payload, (uint16_t)sizeof(payload)); \
    }

static bool zdt_build_motor_enable_frame(uint8_t addr,              //电机地址
                                         ZDT_EnableState enable_state,//使能状态
                                         ZDT_SyncFlag sync_flag,     //同步标志
                                         uint8_t *frame,             //输出的完整命令帧缓冲区
                                         uint16_t frame_capacity,    //缓冲区最大容量
                                         uint16_t *frame_size)       //输出的完整帧长度
{
    const uint8_t payload[] =
    {
        /* 5.3.2 固定格式：电机使能辅助码 + 使能状态 + 同步标志。 */
        ZDT_ASSIST_ENABLE,
        (uint8_t)enable_state,
        (uint8_t)sync_flag
    };

    if ((!zdt_is_flag((uint8_t)enable_state)) || (!zdt_is_flag((uint8_t)sync_flag)))
    {
        return false;
    }

    return zdt_build_frame(addr,
                           false,
                           ZDT_CODE_MOTOR_ENABLE,
                           payload,
                           (uint16_t)sizeof(payload),
                           frame,
                           frame_capacity,
                           frame_size);
}

static bool zdt_build_emm_speed_frame(uint8_t addr,              //电机地址
                                      const ZDT_EmmSpeedCmd *cmd,//Emm 速度模式参数结构体
                                      uint8_t *frame,            //输出的完整命令帧缓冲区
                                      uint16_t frame_capacity,   //缓冲区最大容量
                                      uint16_t *frame_size)      //输出的完整帧长度
{
    uint8_t payload[5];

    if (cmd == NULL)
    {
        return false;
    }

    if ((!zdt_is_flag((uint8_t)cmd->direction)) ||
        (!zdt_is_emm_speed(cmd->speed_rpm)) ||
        (!zdt_is_flag((uint8_t)cmd->sync_flag)))
    {
        return false;
    }

    /* Emm 速度模式：方向 + 速度(2Byte) + 加速度(1Byte) + 同步标志。 */
    payload[0] = (uint8_t)cmd->direction;
    zdt_write_u16_be(&payload[1], cmd->speed_rpm);
    payload[3] = cmd->acceleration;
    payload[4] = (uint8_t)cmd->sync_flag;

    return zdt_build_frame(addr,
                           false,
                           ZDT_CODE_EMM_SPEED,
                           payload,
                           (uint16_t)sizeof(payload),
                           frame,
                           frame_capacity,
                           frame_size);
}

static bool zdt_build_emm_position_frame(uint8_t addr,                 //电机地址
                                         const ZDT_EmmPositionCmd *cmd,//Emm 位置模式参数结构体
                                         uint8_t *frame,               //输出的完整命令帧缓冲区
                                         uint16_t frame_capacity,      //缓冲区最大容量
                                         uint16_t *frame_size)         //输出的完整帧长度
{
    uint8_t payload[10];

    if (cmd == NULL)
    {
        return false;
    }

    if ((!zdt_is_flag((uint8_t)cmd->direction)) ||
        (!zdt_is_emm_speed(cmd->speed_rpm)) ||
        (!zdt_is_position_mode((uint8_t)cmd->position_mode)) ||
        (!zdt_is_flag((uint8_t)cmd->sync_flag)))
    {
        return false;
    }

    /* Emm 位置模式：方向 + 速度(2B) + 加速度(1B) + 脉冲数(4B) + 运动模式 + 同步标志。 */
    payload[0] = (uint8_t)cmd->direction;
    zdt_write_u16_be(&payload[1], cmd->speed_rpm);
    payload[3] = cmd->acceleration;
    zdt_write_u32_be(&payload[4], cmd->pulse_count);
    payload[8] = (uint8_t)cmd->position_mode;
    payload[9] = (uint8_t)cmd->sync_flag;

    return zdt_build_frame(addr,
                           false,
                           ZDT_CODE_EMM_POSITION,
                           payload,
                           (uint16_t)sizeof(payload),
                           frame,
                           frame_capacity,
                           frame_size);
}

static bool zdt_build_stop_frame(uint8_t addr,           //电机地址
                                 ZDT_SyncFlag sync_flag, //同步标志
                                 uint8_t *frame,         //输出的完整命令帧缓冲区
                                 uint16_t frame_capacity,//缓冲区最大容量
                                 uint16_t *frame_size)   //输出的完整帧长度
{
    const uint8_t payload[] =
    {
        /* 5.3.13 固定格式：98 + 同步标志。 */
        0x98U,
        (uint8_t)sync_flag
    };

    if (!zdt_is_flag((uint8_t)sync_flag))
    {
        return false;
    }

    return zdt_build_frame(addr,
                           false,
                           ZDT_CODE_STOP_MOTOR,
                           payload,
                           (uint16_t)sizeof(payload),
                           frame,
                           frame_capacity,
                           frame_size);
}

/* ==================== 5.2 触发动作命令 ==================== */
ZDT_DEFINE_SIMPLE_ACTION(ZDT_CalibrateEncoder,       ZDT_CODE_CALIBRATE_ENCODER,       ZDT_ASSIST_CALIBRATE_ENCODER)
ZDT_DEFINE_SIMPLE_ACTION(ZDT_RebootMotor,            ZDT_CODE_REBOOT_MOTOR,            ZDT_ASSIST_REBOOT_MOTOR)
ZDT_DEFINE_SIMPLE_ACTION(ZDT_ClearCurrentPosition,   ZDT_CODE_CLEAR_CURRENT_POSITION,  ZDT_ASSIST_CLEAR_CURRENT_POSITION)
ZDT_DEFINE_SIMPLE_ACTION(ZDT_ClearProtection,        ZDT_CODE_CLEAR_PROTECTION,        ZDT_ASSIST_CLEAR_PROTECTION)
ZDT_DEFINE_SIMPLE_ACTION(ZDT_RestoreFactorySettings, ZDT_CODE_RESTORE_FACTORY,         ZDT_ASSIST_RESTORE_FACTORY)

/* ==================== 5.3.1 多电机命令 Builder ==================== */
zdt_cmd_status_t ZDT_MultiCmdBuilderInit(ZDT_MultiCmdBuilder *builder, //多电机命令构建器对象
                             uint8_t *buffer,              //外部提供的总帧缓冲区
                             uint16_t capacity)            //缓冲区总容量
{
    if ((builder == NULL) || (buffer == NULL) || (capacity < ZDT_MULTI_CMD_MIN_CAPACITY))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    builder->buffer = buffer;
    builder->capacity = capacity;
    builder->length = 4U;
    builder->finalized = false;

    /* 多电机命令头固定为：00 AA 长度高字节 长度低字节。 */
    builder->buffer[0] = 0x00U;
    builder->buffer[1] = ZDT_CODE_MULTI_MOTOR;
    builder->buffer[2] = 0x00U;
    builder->buffer[3] = 0x00U;
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_MultiCmdBuilderReset(ZDT_MultiCmdBuilder *builder) //要复位的构建器对象
{
    if ((builder == NULL) || (builder->buffer == NULL) || (builder->capacity < ZDT_MULTI_CMD_MIN_CAPACITY))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    return ZDT_MultiCmdBuilderInit(builder, builder->buffer, builder->capacity);
}

zdt_cmd_status_t ZDT_MultiCmdBuilderAppendRaw(ZDT_MultiCmdBuilder *builder,      //多电机命令构建器对象
                                  const uint8_t *sub_frame,          //单个子命令帧首地址
                                  uint16_t sub_frame_size)           //子命令帧长度
{
    if ((builder == NULL) || (builder->buffer == NULL) || (sub_frame == NULL))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if ((builder->finalized) ||
        (sub_frame_size < 3U) ||
        (sub_frame[sub_frame_size - 1U] != ZDT_FIXED_CHECKSUM) ||
        (!zdt_is_valid_addr(sub_frame[0])) ||
        ((uint16_t)(builder->length + sub_frame_size + 1U) > builder->capacity))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    /* 子命令原样拷贝进总帧中，保留各自结尾的 0x6B。 */
    (void)memcpy(&builder->buffer[builder->length], sub_frame, sub_frame_size);
    builder->length = (uint16_t)(builder->length + sub_frame_size);
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_MultiCmdBuilderAppendRead(ZDT_MultiCmdBuilder *builder, //多电机命令构建器对象
                                   uint8_t addr,                 //子命令目标电机地址
                                   uint8_t code)                 //读取类功能码
{
    uint8_t frame[3];
    uint16_t frame_size;

    if (!zdt_build_frame(addr, false, code, NULL, 0U, frame, (uint16_t)sizeof(frame), &frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    /* 读取类子命令没有数据区，因此直接复用统一组帧器。 */
    return ZDT_MultiCmdBuilderAppendRaw(builder, frame, frame_size);
}

zdt_cmd_status_t ZDT_MultiCmdBuilderAppendEnable(ZDT_MultiCmdBuilder *builder, //多电机命令构建器对象
                                     uint8_t addr,                 //子命令目标电机地址
                                     ZDT_EnableState enable_state, //使能状态
                                     ZDT_SyncFlag sync_flag)       //同步标志
{
    uint8_t frame[6];
    uint16_t frame_size;

    if (!zdt_build_motor_enable_frame(addr,
                                      enable_state,
                                      sync_flag,
                                      frame,
                                      (uint16_t)sizeof(frame),
                                      &frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    return ZDT_MultiCmdBuilderAppendRaw(builder, frame, frame_size);
}

zdt_cmd_status_t ZDT_MultiCmdBuilderAppendEmmSpeed(ZDT_MultiCmdBuilder *builder, //多电机命令构建器对象
                                       uint8_t addr,                 //子命令目标电机地址
                                       const ZDT_EmmSpeedCmd *cmd)   //Emm 速度模式参数结构体
{
    uint8_t frame[8];
    uint16_t frame_size;

    if (!zdt_build_emm_speed_frame(addr, cmd, frame, (uint16_t)sizeof(frame), &frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    return ZDT_MultiCmdBuilderAppendRaw(builder, frame, frame_size);
}

zdt_cmd_status_t ZDT_MultiCmdBuilderAppendEmmPosition(ZDT_MultiCmdBuilder *builder,   //多电机命令构建器对象
                                          uint8_t addr,                   //子命令目标电机地址
                                          const ZDT_EmmPositionCmd *cmd)  //Emm 位置模式参数结构体
{
    uint8_t frame[13];
    uint16_t frame_size;

    if (!zdt_build_emm_position_frame(addr, cmd, frame, (uint16_t)sizeof(frame), &frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    return ZDT_MultiCmdBuilderAppendRaw(builder, frame, frame_size);
}

zdt_cmd_status_t ZDT_MultiCmdBuilderAppendStop(ZDT_MultiCmdBuilder *builder, //多电机命令构建器对象
                                   uint8_t addr,                 //子命令目标电机地址
                                   ZDT_SyncFlag sync_flag)       //同步标志
{
    uint8_t frame[5];
    uint16_t frame_size;

    if (!zdt_build_stop_frame(addr, sync_flag, frame, (uint16_t)sizeof(frame), &frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    return ZDT_MultiCmdBuilderAppendRaw(builder, frame, frame_size);
}

zdt_cmd_status_t ZDT_MultiCmdBuilderFinalize(ZDT_MultiCmdBuilder *builder, //多电机命令构建器对象
                                 uint16_t *out_size)           //输出的最终总帧长度
{
    if ((builder == NULL) || (builder->buffer == NULL))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (!builder->finalized)
    {
        uint16_t total_size;

        if ((uint16_t)(builder->length + 1U) > builder->capacity)
        {
            return ZDT_CMD_STATUS_RESOURCE;
        }

        /* 多电机命令最终格式：00 AA 总长度(2B) + 子命令... + 0x6B。 */
        builder->buffer[builder->length] = ZDT_FIXED_CHECKSUM;
        builder->length = (uint16_t)(builder->length + 1U);
        total_size = builder->length;
        /* 总长度字段包含帧头、所有子命令以及最后一个 0x6B。 */
        builder->buffer[2] = (uint8_t)(total_size >> 8);
        builder->buffer[3] = (uint8_t)(total_size & 0xFFU);
        builder->finalized = true;
    }

    if (out_size != NULL)
    {
        *out_size = builder->length;
    }

    return ZDT_CMD_STATUS_OK;
}

bool ZDT_MultiCmdBuilderSend(ZDT_MultiCmdBuilder *builder) //要发送的多电机命令构建器对象
{
    if (ZDT_CMD_STATUS_OK != ZDT_MultiCmdBuilderFinalize(builder, NULL))
    {
        return false;
    }

    return zdt_capture_frame_data(builder->buffer, builder->length);
}

/* ==================== 5.3 电机控制命令 ==================== */
bool ZDT_SetMotorEnable(uint8_t addr,               //电机地址
                        ZDT_EnableState enable_state,//使能状态
                        ZDT_SyncFlag sync_flag)     //同步标志
{
    uint8_t frame[6];
    uint16_t frame_size;

    if (!zdt_build_motor_enable_frame(addr,
                                      enable_state,
                                      sync_flag,
                                      frame,
                                      (uint16_t)sizeof(frame),
                                      &frame_size))
    {
        return false;
    }

    return zdt_capture_frame_data(frame, frame_size);
}

bool ZDT_EmmRunSpeed(uint8_t addr,               //电机地址
                     const ZDT_EmmSpeedCmd *cmd) //Emm 速度模式参数结构体
{
    uint8_t frame[8];
    uint16_t frame_size;

    if (!zdt_build_emm_speed_frame(addr, cmd, frame, (uint16_t)sizeof(frame), &frame_size))
    {
        return false;
    }

    return zdt_capture_frame_data(frame, frame_size);
}

bool ZDT_EmmRunPosition(uint8_t addr,                  //电机地址
                        const ZDT_EmmPositionCmd *cmd) //Emm 位置模式参数结构体
{
    uint8_t frame[13];
    uint16_t frame_size;

    if (!zdt_build_emm_position_frame(addr, cmd, frame, (uint16_t)sizeof(frame), &frame_size))
    {
        return false;
    }

    return zdt_capture_frame_data(frame, frame_size);
}

bool ZDT_StopMotor(uint8_t addr,           //电机地址
                   ZDT_SyncFlag sync_flag) //同步标志
{
    uint8_t frame[5];
    uint16_t frame_size;

    if (!zdt_build_stop_frame(addr, sync_flag, frame, (uint16_t)sizeof(frame), &frame_size))
    {
        return false;
    }

    return zdt_capture_frame_data(frame, frame_size);
}

bool ZDT_TriggerMultiSync(void)
{
    const uint8_t payload[] = { ZDT_ASSIST_TRIGGER_MULTI_SYNC };

    /* 该命令必须广播发送，用于让之前缓存的同步命令同时生效。 */
    return zdt_send_frame(0x00U, true, ZDT_CODE_TRIGGER_MULTI_SYNC, payload, (uint16_t)sizeof(payload));
}

/* ==================== 5.4 回零相关命令 ==================== */
bool ZDT_SetSingleTurnHomeZero(uint8_t addr,      //电机地址
                               bool save_to_flash)//是否保存到电机 Flash
{
    const uint8_t payload[] =
    {
        /* 5.4.1 固定格式：88 + 保存标志。 */
        ZDT_ASSIST_SET_SINGLE_TURN_ZERO,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO)
    };

    return zdt_send_frame(addr, false, ZDT_CODE_SET_SINGLE_TURN_ZERO, payload, (uint16_t)sizeof(payload));
}

bool ZDT_StartHoming(uint8_t addr,            //电机地址
                     uint8_t homing_mode,     //回零模式编号
                     ZDT_SyncFlag sync_flag)  //同步标志
{
    const uint8_t payload[] =
    {
        /* 5.4.2：回零模式 + 同步标志。 */
        homing_mode,
        (uint8_t)sync_flag
    };

    if ((!zdt_is_homing_mode(homing_mode)) || (!zdt_is_flag((uint8_t)sync_flag)))
    {
        return false;
    }

    return zdt_send_frame(addr, false, ZDT_CODE_START_HOMING, payload, (uint16_t)sizeof(payload));
}

bool ZDT_AbortHoming(uint8_t addr) //电机地址
{
    const uint8_t payload[] = { ZDT_ASSIST_ABORT_HOMING };

    return zdt_send_frame(addr, false, ZDT_CODE_ABORT_HOMING, payload, (uint16_t)sizeof(payload));
}

/* ==================== 5.5 / 5.6 读取类命令 ==================== */
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestHomingStatus,              ZDT_CODE_READ_HOMING_STATUS)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestHomingParams,              ZDT_CODE_READ_HOMING_PARAMS)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestVersionInfo,               ZDT_CODE_READ_VERSION)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestPhaseResistanceInductance, ZDT_CODE_READ_PHASE_RL)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestBusVoltage,                ZDT_CODE_READ_BUS_VOLTAGE)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestBusCurrent,                ZDT_CODE_READ_BUS_CURRENT)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestPhaseCurrent,              ZDT_CODE_READ_PHASE_CURRENT)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestEncoderValue,              ZDT_CODE_READ_ENCODER_VALUE)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestInputPulseCount,           ZDT_CODE_READ_INPUT_PULSE)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestTargetPosition,            ZDT_CODE_READ_TARGET_POSITION)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestRealtimeTargetPosition,    ZDT_CODE_READ_REALTIME_TARGET)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestRealtimeSpeed,             ZDT_CODE_READ_REALTIME_SPEED)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestDriverTemperature,         ZDT_CODE_READ_DRIVER_TEMPERATURE)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestRealtimePosition,          ZDT_CODE_READ_REALTIME_POSITION)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestPositionError,             ZDT_CODE_READ_POSITION_ERROR)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestMotorStatus,               ZDT_CODE_READ_MOTOR_STATUS)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestHomingAndMotorStatus,      ZDT_CODE_READ_HOMING_MOTOR_STATUS)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestIoState,                   ZDT_CODE_READ_IO_STATE)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestOptionStatus,              ZDT_CODE_READ_OPTION_STATUS)
ZDT_DEFINE_SIMPLE_READ(ZDT_EmmRequestPid,                    ZDT_CODE_READ_PID)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestPositionWindow,            ZDT_CODE_READ_POSITION_WINDOW)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestProtectionThresholds,      ZDT_CODE_READ_OVER_TEMP_CURRENT)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestHeartbeatTime,             ZDT_CODE_READ_HEARTBEAT_TIME)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestIntegralLimit,             ZDT_CODE_READ_INTEGRAL_LIMIT)
ZDT_DEFINE_SIMPLE_READ(ZDT_RequestCollisionReturnAngle,      ZDT_CODE_READ_COLLISION_RETURN)

bool ZDT_SetHomingParams(uint8_t addr,               //电机地址
                         const ZDT_HomingParams *params)//回零参数结构体
{
    uint8_t payload[17];

    if (params == NULL)
    {
        return false;
    }

    if ((!zdt_is_homing_mode(params->homing_mode)) ||
        (!zdt_is_flag((uint8_t)params->homing_direction)) ||
        (!zdt_is_emm_speed(params->homing_speed_rpm)) ||
        (!zdt_is_emm_speed(params->collision_speed_rpm)) ||
        (!zdt_is_current_ma(params->collision_current_ma)))
    {
        return false;
    }

    /* 5.4.6 参数顺序：
     * 辅助码 AE + 是否存储 + 回零模式 + 回零方向 + 回零速度(2B)
     * + 回零超时时间(4B) + 碰撞检测转速(2B) + 碰撞检测电流(2B)
     * + 碰撞检测时间(2B) + O_POT_En。 */
    payload[0] = ZDT_ASSIST_SET_HOMING_PARAMS;
    payload[1] = (uint8_t)(params->save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    payload[2] = params->homing_mode;
    payload[3] = (uint8_t)params->homing_direction;
    zdt_write_u16_be(&payload[4], params->homing_speed_rpm);
    zdt_write_u32_be(&payload[6], params->homing_timeout_ms);
    zdt_write_u16_be(&payload[10], params->collision_speed_rpm);
    zdt_write_u16_be(&payload[12], params->collision_current_ma);
    zdt_write_u16_be(&payload[14], params->collision_time_ms);
    payload[16] = (uint8_t)(params->enable_limit_input ? 0x01U : 0x00U);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_HOMING_PARAMS, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetPeriodicReport(uint8_t addr,         //电机地址
                           uint8_t info_code,    //要定时返回的信息功能码
                           uint16_t period_ms)   //返回周期，单位 ms
{
    uint8_t payload[4];

    /* 这里只允许手册列出的那些“可定时主动上报”的读取功能码。 */
    if (!zdt_is_periodic_report_code(info_code))
    {
        return false;
    }

    /* 定时返回：辅助码 18 + 信息功能码 + 定时时间(2B)。 */
    payload[0] = ZDT_ASSIST_PERIODIC_REPORT;
    payload[1] = info_code;
    zdt_write_u16_be(&payload[2], period_ms);

    return zdt_send_frame(addr, false, ZDT_CODE_PERIODIC_REPORT, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetMotorAddress(uint8_t addr,         //当前电机地址
                         bool save_to_flash,   //是否保存到电机 Flash
                         uint8_t new_addr)     //新地址
{
    const uint8_t payload[] =
    {
        /* 5.6.1：辅助码 4B + 保存标志 + 新地址。 */
        ZDT_ASSIST_SET_MOTOR_ADDRESS,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        new_addr
    };

    if (!zdt_is_valid_addr(new_addr))
    {
        return false;
    }

    return zdt_send_frame(addr, false, ZDT_CODE_SET_MOTOR_ADDRESS, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetMicrostep(uint8_t addr,          //电机地址
                      bool save_to_flash,    //是否保存到电机 Flash
                      uint8_t microstep)     //细分参数原始值
{
    const uint8_t payload[] =
    {
        /* 5.6.2 手册示例中辅助码固定为 8A。 */
        0x8AU,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        microstep
    };

    return zdt_send_frame(addr, false, ZDT_CODE_SET_MICROSTEP, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetPowerLossFlag(uint8_t addr,         //电机地址
                          bool power_loss_flag) //掉电标志开关
{
    const uint8_t payload[] =
    {
        /* 掉电标志命令没有保存位，数据区仅 1 字节。 */
        (uint8_t)(power_loss_flag ? 0x01U : 0x00U)
    };

    return zdt_send_frame(addr, false, ZDT_CODE_SET_POWER_LOSS_FLAG, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetMotorType(uint8_t addr,             //电机地址
                      bool save_to_flash,       //是否保存到电机 Flash
                      uint8_t motor_type_code)  //电机类型码
{
    const uint8_t payload[] =
    {
        /* 5.6.5：辅助码 35 + 保存标志 + 电机类型码。 */
        ZDT_ASSIST_SET_MOTOR_TYPE,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        motor_type_code
    };

    if (!zdt_is_motor_type_code(motor_type_code))
    {
        return false;
    }

    return zdt_send_frame(addr, false, ZDT_CODE_SET_MOTOR_TYPE, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetFirmwareType(uint8_t addr,                  //电机地址
                         bool save_to_flash,            //是否保存到电机 Flash
                         ZDT_FirmwareType firmware_type)//固件类型
{
    const uint8_t payload[] =
    {
        /* 5.6.6：辅助码 69 + 保存标志 + 固件类型。 */
        ZDT_ASSIST_SET_FIRMWARE_TYPE,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        (uint8_t)firmware_type
    };

    if ((uint8_t)firmware_type > 0x02U)
    {
        return false;
    }

    return zdt_send_frame(addr, false, ZDT_CODE_SET_FIRMWARE_TYPE, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetControlMode(uint8_t addr,                //电机地址
                        bool save_to_flash,          //是否保存到电机 Flash
                        ZDT_ControlMode control_mode)//控制模式
{
    const uint8_t payload[] =
    {
        /* 5.6.7：辅助码 A6 + 保存标志 + 开环/闭环。 */
        ZDT_ASSIST_SET_CONTROL_MODE,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        (uint8_t)control_mode
    };

    if (!zdt_is_flag((uint8_t)control_mode))
    {
        return false;
    }

    return zdt_send_frame(addr, false, ZDT_CODE_SET_CONTROL_MODE, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetMotorDirection(uint8_t addr,           //电机地址
                           bool save_to_flash,     //是否保存到电机 Flash
                           ZDT_Direction direction)//电机正方向
{
    const uint8_t payload[] =
    {
        /* 5.6.8：辅助码 60 + 保存标志 + 方向。 */
        ZDT_ASSIST_SET_MOTOR_DIRECTION,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        (uint8_t)direction
    };

    if (!zdt_is_flag((uint8_t)direction))
    {
        return false;
    }

    return zdt_send_frame(addr, false, ZDT_CODE_SET_MOTOR_DIRECTION, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetButtonLock(uint8_t addr,         //电机地址
                       bool save_to_flash,   //是否保存到电机 Flash
                       bool lock_button)     //是否锁定按键
{
    const uint8_t payload[] =
    {
        /* 5.6.9：辅助码 B3 + 保存标志 + 锁键使能。 */
        ZDT_ASSIST_SET_BUTTON_LOCK,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        (uint8_t)(lock_button ? 0x01U : 0x00U)
    };

    return zdt_send_frame(addr, false, ZDT_CODE_SET_BUTTON_LOCK, payload, (uint16_t)sizeof(payload));
}

bool ZDT_EmmSetSpeedInputScale10x(uint8_t addr,      //电机地址
                                  bool save_to_flash,//是否保存到电机 Flash
                                  bool enable_scale) //是否启用速度值缩小 10 倍输入
{
    const uint8_t payload[] =
    {
        /* 5.6.11：辅助码 71 + 保存标志 + 是否缩小 10 倍输入。 */
        ZDT_ASSIST_SET_SPEED_SCALE_10X,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        (uint8_t)(enable_scale ? 0x01U : 0x00U)
    };

    return zdt_send_frame(addr, false, ZDT_CODE_SET_SPEED_SCALE_10X, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetOpenLoopCurrent(uint8_t addr,         //电机地址
                            bool save_to_flash,   //是否保存到电机 Flash
                            uint16_t current_ma)  //开环工作电流，单位 mA
{
    uint8_t payload[4];

    if (!zdt_is_current_ma(current_ma))
    {
        return false;
    }

    /* 5.6.12：辅助码 33 + 保存标志 + 电流值(2B)。 */
    payload[0] = ZDT_ASSIST_SET_OPEN_LOOP_CURRENT;
    payload[1] = (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u16_be(&payload[2], current_ma);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_OPEN_LOOP_CURRENT, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetClosedLoopMaxCurrent(uint8_t addr,        //电机地址
                                 bool save_to_flash,  //是否保存到电机 Flash
                                 uint16_t current_ma) //闭环最大电流，单位 mA
{
    uint8_t payload[4];

    if (!zdt_is_current_ma(current_ma))
    {
        return false;
    }

    /* 5.6.13：辅助码 66 + 保存标志 + 最大电流(2B)。 */
    payload[0] = ZDT_ASSIST_SET_CLOSED_LOOP_CURRENT;
    payload[1] = (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u16_be(&payload[2], current_ma);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_CLOSED_LOOP_CURRENT, payload, (uint16_t)sizeof(payload));
}

bool ZDT_EmmSetPid(uint8_t addr,                //电机地址
                   const ZDT_EmmPidParams *params)//PID 参数结构体
{
    uint8_t payload[14];

    if (params == NULL)
    {
        return false;
    }

    /* Emm PID：辅助码 C3 + 是否存储 + Kp(4B) + Ki(4B) + Kd(4B)。 */
    payload[0] = ZDT_ASSIST_SET_PID;
    payload[1] = (uint8_t)(params->save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u32_be(&payload[2], params->kp);
    zdt_write_u32_be(&payload[6], params->ki);
    zdt_write_u32_be(&payload[10], params->kd);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_PID, payload, (uint16_t)sizeof(payload));
}

bool ZDT_RequestDmx512Params(uint8_t addr) //电机地址
{
    const uint8_t payload[] = { ZDT_ASSIST_READ_DMX512_PARAMS };

    /* 读取 DMX512 参数时，数据区只包含固定辅助码 78。 */
    return zdt_send_frame(addr, false, ZDT_CODE_READ_DMX512_PARAMS, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetDmx512Params(uint8_t addr,                 //电机地址
                         const ZDT_Dmx512Params *params)//DMX512 参数结构体
{
    uint8_t payload[16];

    if (params == NULL)
    {
        return false;
    }

    if ((!zdt_is_dmx_channel_count(params->total_channel_count)) ||
        (!zdt_is_dmx_channels_per_motor(params->channels_per_motor)) ||
        (!zdt_is_dmx_motion_mode(params->motion_mode)) ||
        (!zdt_is_emm_speed(params->single_channel_speed_rpm)) ||
        (params->single_channel_speed_rpm == 0U) ||
        (params->acceleration == 0U) ||
        (params->dual_channel_speed_step_rpm == 0U) ||
        (params->dual_channel_motion_step == 0U))
    {
        return false;
    }

    /* DMX512 参数：辅助码 90 + 是否存储 + 总通道数(2B) + 每电机通道数
     * + 运动模式 + 单通道速度(2B) + 加速度(2B) + 双通道速度步长(2B)
     * + 双通道运动步长(4B)。 */
    payload[0] = ZDT_ASSIST_SET_DMX512_PARAMS;
    payload[1] = (uint8_t)(params->save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u16_be(&payload[2], params->total_channel_count);
    payload[4] = params->channels_per_motor;
    payload[5] = params->motion_mode;
    zdt_write_u16_be(&payload[6], params->single_channel_speed_rpm);
    zdt_write_u16_be(&payload[8], params->acceleration);
    zdt_write_u16_be(&payload[10], params->dual_channel_speed_step_rpm);
    zdt_write_u32_be(&payload[12], params->dual_channel_motion_step);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_DMX512_PARAMS, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetPositionWindow(uint8_t addr,                //电机地址
                           bool save_to_flash,          //是否保存到电机 Flash
                           uint16_t position_window_0p1deg)//位置到达窗口，单位 0.1°
{
    uint8_t payload[4];

    /* 5.6.21：辅助码 07 + 保存标志 + 窗口值(2B, 单位 0.1°)。 */
    payload[0] = ZDT_ASSIST_SET_POSITION_WINDOW;
    payload[1] = (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u16_be(&payload[2], position_window_0p1deg);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_POSITION_WINDOW, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetProtectionThresholds(uint8_t addr,            //电机地址
                                 bool save_to_flash,      //是否保存到电机 Flash
                                 uint16_t over_temp_celsius,//过热阈值，单位摄氏度
                                 uint16_t over_current_ma, //过流阈值，单位 mA
                                 uint16_t detect_time_ms)  //检测时间，单位 ms
{
    uint8_t payload[8];

    /* 5.6.23：辅助码 56 + 保存标志 + 过热阈值(2B) + 过流阈值(2B) + 检测时间(2B)。 */
    payload[0] = ZDT_ASSIST_SET_PROTECTION_THRESHOLDS;
    payload[1] = (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u16_be(&payload[2], over_temp_celsius);
    zdt_write_u16_be(&payload[4], over_current_ma);
    zdt_write_u16_be(&payload[6], detect_time_ms);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_PROTECTION_THRESHOLDS, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetHeartbeatTime(uint8_t addr,              //电机地址
                          bool save_to_flash,        //是否保存到电机 Flash
                          uint32_t heartbeat_time_ms)//心跳保护时间，单位 ms
{
    uint8_t payload[6];

    /* 5.6.25：辅助码 38 + 保存标志 + 心跳时间(4B)。 */
    payload[0] = ZDT_ASSIST_SET_HEARTBEAT_TIME;
    payload[1] = (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u32_be(&payload[2], heartbeat_time_ms);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_HEARTBEAT_TIME, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetIntegralLimit(uint8_t addr,             //电机地址
                          bool save_to_flash,       //是否保存到电机 Flash
                          uint32_t integral_limit)  //积分限幅/刚性系数原始值
{
    uint8_t payload[6];

    /* 5.6.27：辅助码 57 + 保存标志 + 积分限幅/刚性系数(4B)。 */
    payload[0] = ZDT_ASSIST_SET_INTEGRAL_LIMIT;
    payload[1] = (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u32_be(&payload[2], integral_limit);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_INTEGRAL_LIMIT, payload, (uint16_t)sizeof(payload));
}

bool ZDT_SetCollisionReturnAngle(uint8_t addr,         //电机地址
                                 bool save_to_flash,   //是否保存到电机 Flash
                                 uint16_t angle_0p1deg)//返回角度，单位 0.1°
{
    uint8_t payload[4];

    /* 5.6.29：辅助码 AC + 保存标志 + 返回角度(2B, 单位 0.1°)。 */
    payload[0] = ZDT_ASSIST_SET_COLLISION_RETURN;
    payload[1] = (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    zdt_write_u16_be(&payload[2], angle_0p1deg);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_COLLISION_RETURN, payload, (uint16_t)sizeof(payload));
}

bool ZDT_BroadcastRequestAddress(void)
{
    /* 5.6.30 明确要求广播发送，因此地址固定为 0。 */
    return zdt_send_frame(0x00U, true, ZDT_CODE_READ_BROADCAST_ADDRESS, NULL, 0U);
}

bool ZDT_SetParameterLockLevel(uint8_t addr,         //电机地址
                               bool save_to_flash,   //是否保存到电机 Flash
                               uint8_t lock_level)   //参数锁定等级
{
    const uint8_t payload[] =
    {
        /* 5.6.31：辅助码 4B + 保存标志 + 锁定等级。 */
        ZDT_ASSIST_SET_PARAM_LOCK_LEVEL,
        (uint8_t)(save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO),
        lock_level
    };

    if (!zdt_is_param_lock_level(lock_level))
    {
        return false;
    }

    return zdt_send_frame(addr, false, ZDT_CODE_SET_PARAM_LOCK_LEVEL, payload, (uint16_t)sizeof(payload));
}

bool ZDT_EmmSetAutoRun(uint8_t addr,                    //电机地址
                       const ZDT_EmmAutoRunConfig *config)//上电自动运行参数结构体
{
    uint8_t payload[7];

    if (config == NULL)
    {
        return false;
    }

    if ((!zdt_is_flag((uint8_t)config->direction)) || (!zdt_is_emm_speed(config->speed_rpm)))
    {
        return false;
    }

    /* Emm 上电自动运行：辅助码 1C + 清除/存储 + 方向 + 速度(2B) + 加速度 + En 控制启停。 */
    payload[0] = ZDT_ASSIST_EMM_AUTO_RUN;
    payload[1] = (uint8_t)(config->save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    payload[2] = (uint8_t)config->direction;
    zdt_write_u16_be(&payload[3], config->speed_rpm);
    payload[5] = config->acceleration;
    payload[6] = (uint8_t)(config->enable_en_pin_control ? 0x01U : 0x00U);

    return zdt_send_frame(addr, false, ZDT_CODE_EMM_AUTO_RUN, payload, (uint16_t)sizeof(payload));
}

/* ==================== 5.8 Emm 系统参数命令 ==================== */
bool ZDT_EmmRequestAllSystemStatus(uint8_t addr) //电机地址
{
    const uint8_t payload[] = { ZDT_ASSIST_READ_ALL_SYSTEM_STATUS };

    /* 读取系统状态时，数据区只带固定辅助码 7A。 */
    return zdt_send_frame(addr, false, ZDT_CODE_READ_ALL_SYSTEM_STATUS, payload, (uint16_t)sizeof(payload));
}

bool ZDT_EmmRequestDriveConfig(uint8_t addr) //电机地址
{
    const uint8_t payload[] = { ZDT_ASSIST_READ_DRIVE_CONFIG };

    /* 读取驱动配置时，数据区只带固定辅助码 6C。 */
    return zdt_send_frame(addr, false, ZDT_CODE_READ_DRIVE_CONFIG, payload, (uint16_t)sizeof(payload));
}

bool ZDT_EmmSetDriveConfig(uint8_t addr,                 //电机地址
                           const ZDT_EmmDriveConfig *config)//驱动配置参数结构体
{
    uint8_t payload[30];

    if (config == NULL)
    {
        return false;
    }

    if ((!zdt_is_motor_type_code(config->motor_type_code)) ||
        (!zdt_is_port_mode(config->pulse_port_mode)) ||
        (!zdt_is_port_mode(config->comm_port_mode)) ||
        (!zdt_is_en_active_level(config->en_pin_active_level)) ||
        (!zdt_is_flag(config->dir_pin_active_level)) ||
        (!zdt_is_current_ma(config->open_loop_current_ma)) ||
        (!zdt_is_current_ma(config->closed_loop_stall_current_ma)) ||
        (!zdt_is_current_ma(config->closed_loop_max_output_voltage)) ||
        (!zdt_is_uart_baud_index(config->uart_baud_index)) ||
        (!zdt_is_can_baud_index(config->can_baud_index)) ||
        (!zdt_is_checksum_mode(config->checksum_mode)) ||
        (!zdt_is_response_mode(config->response_mode)) ||
        (!zdt_is_stall_protection_mode(config->stall_protection_mode)) ||
        (!zdt_is_emm_speed(config->stall_detect_speed_rpm)) ||
        (!zdt_is_current_ma(config->stall_detect_current_ma)))
    {
        return false;
    }

    /* Emm 驱动配置：辅助码 D1 + 是否存储 + 21 个参数，字段顺序严格按照 5.8.6。 */
    payload[0] = ZDT_ASSIST_SET_DRIVE_CONFIG;
    payload[1] = (uint8_t)(config->save_to_flash ? ZDT_SAVE_YES : ZDT_SAVE_NO);
    /* 前 8 个字节是基础接口和驱动行为配置。 */
    payload[2] = config->motor_type_code;
    payload[3] = config->pulse_port_mode;
    payload[4] = config->comm_port_mode;
    payload[5] = config->en_pin_active_level;
    payload[6] = config->dir_pin_active_level;
    payload[7] = config->microstep;
    payload[8] = (uint8_t)(config->microstep_interpolation ? 0x01U : 0x00U);
    payload[9] = config->reserved0;
    /* 中间这几项是电流、电压与通讯参数。 */
    zdt_write_u16_be(&payload[10], config->open_loop_current_ma);
    zdt_write_u16_be(&payload[12], config->closed_loop_stall_current_ma);
    zdt_write_u16_be(&payload[14], config->closed_loop_max_output_voltage);
    payload[16] = config->uart_baud_index;
    payload[17] = config->can_baud_index;
    payload[18] = config->reserved1;
    payload[19] = config->checksum_mode;
    payload[20] = config->response_mode;
    payload[21] = config->stall_protection_mode;
    /* 最后是堵转检测阈值以及位置到达窗口。 */
    zdt_write_u16_be(&payload[22], config->stall_detect_speed_rpm);
    zdt_write_u16_be(&payload[24], config->stall_detect_current_ma);
    zdt_write_u16_be(&payload[26], config->stall_detect_time_ms);
    zdt_write_u16_be(&payload[28], config->position_window_0p1deg);

    return zdt_send_frame(addr, false, ZDT_CODE_SET_DRIVE_CONFIG, payload, (uint16_t)sizeof(payload));
}

/* ==================== Build 前缀构帧接口 ==================== */

zdt_cmd_status_t ZDT_BuildCalibrateEncoderFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_CalibrateEncoder(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRebootMotorFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RebootMotor(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildClearCurrentPositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_ClearCurrentPosition(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildClearProtectionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_ClearProtection(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRestoreFactorySettingsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RestoreFactorySettings(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetMotorEnableFrame(uint8_t addr, ZDT_EnableState enable_state, ZDT_SyncFlag sync_flag, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetMotorEnable(addr, enable_state, sync_flag))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmRunSpeedFrame(uint8_t addr, const ZDT_EmmSpeedCmd *cmd, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmRunSpeed(addr, cmd))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmRunPositionFrame(uint8_t addr, const ZDT_EmmPositionCmd *cmd, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmRunPosition(addr, cmd))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildStopMotorFrame(uint8_t addr, ZDT_SyncFlag sync_flag, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_StopMotor(addr, sync_flag))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildTriggerMultiSyncFrame(uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_TriggerMultiSync())
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetSingleTurnHomeZeroFrame(uint8_t addr, bool save_to_flash, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetSingleTurnHomeZero(addr, save_to_flash))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildStartHomingFrame(uint8_t addr, uint8_t homing_mode, ZDT_SyncFlag sync_flag, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_StartHoming(addr, homing_mode, sync_flag))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildAbortHomingFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_AbortHoming(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestHomingStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestHomingStatus(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestHomingParamsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestHomingParams(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetHomingParamsFrame(uint8_t addr, const ZDT_HomingParams *params, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetHomingParams(addr, params))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetPeriodicReportFrame(uint8_t addr, uint8_t info_code, uint16_t period_ms, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetPeriodicReport(addr, info_code, period_ms))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestVersionInfoFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestVersionInfo(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestPhaseResistanceInductanceFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestPhaseResistanceInductance(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestBusVoltageFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestBusVoltage(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestBusCurrentFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestBusCurrent(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestPhaseCurrentFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestPhaseCurrent(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestEncoderValueFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestEncoderValue(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestInputPulseCountFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestInputPulseCount(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestTargetPositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestTargetPosition(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestRealtimeTargetPositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestRealtimeTargetPosition(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestRealtimeSpeedFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestRealtimeSpeed(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestDriverTemperatureFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestDriverTemperature(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestRealtimePositionFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestRealtimePosition(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestPositionErrorFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestPositionError(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestMotorStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestMotorStatus(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestHomingAndMotorStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestHomingAndMotorStatus(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestIoStateFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestIoState(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetMotorAddressFrame(uint8_t addr, bool save_to_flash, uint8_t new_addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetMotorAddress(addr, save_to_flash, new_addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetMicrostepFrame(uint8_t addr, bool save_to_flash, uint8_t microstep, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetMicrostep(addr, save_to_flash, microstep))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetPowerLossFlagFrame(uint8_t addr, bool power_loss_flag, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetPowerLossFlag(addr, power_loss_flag))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestOptionStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestOptionStatus(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetMotorTypeFrame(uint8_t addr, bool save_to_flash, uint8_t motor_type_code, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetMotorType(addr, save_to_flash, motor_type_code))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetFirmwareTypeFrame(uint8_t addr, bool save_to_flash, ZDT_FirmwareType firmware_type, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetFirmwareType(addr, save_to_flash, firmware_type))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetControlModeFrame(uint8_t addr, bool save_to_flash, ZDT_ControlMode control_mode, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetControlMode(addr, save_to_flash, control_mode))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetMotorDirectionFrame(uint8_t addr, bool save_to_flash, ZDT_Direction direction, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetMotorDirection(addr, save_to_flash, direction))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetButtonLockFrame(uint8_t addr, bool save_to_flash, bool lock_button, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetButtonLock(addr, save_to_flash, lock_button))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmSetSpeedInputScale10xFrame(uint8_t addr, bool save_to_flash, bool enable_scale, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmSetSpeedInputScale10x(addr, save_to_flash, enable_scale))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetOpenLoopCurrentFrame(uint8_t addr, bool save_to_flash, uint16_t current_ma, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetOpenLoopCurrent(addr, save_to_flash, current_ma))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetClosedLoopMaxCurrentFrame(uint8_t addr, bool save_to_flash, uint16_t current_ma, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetClosedLoopMaxCurrent(addr, save_to_flash, current_ma))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmRequestPidFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmRequestPid(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmSetPidFrame(uint8_t addr, const ZDT_EmmPidParams *params, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmSetPid(addr, params))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestDmx512ParamsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestDmx512Params(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetDmx512ParamsFrame(uint8_t addr, const ZDT_Dmx512Params *params, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetDmx512Params(addr, params))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestPositionWindowFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestPositionWindow(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetPositionWindowFrame(uint8_t addr, bool save_to_flash, uint16_t position_window_0p1deg, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetPositionWindow(addr, save_to_flash, position_window_0p1deg))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestProtectionThresholdsFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestProtectionThresholds(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetProtectionThresholdsFrame(uint8_t addr, bool save_to_flash, uint16_t over_temp_celsius, uint16_t over_current_ma, uint16_t detect_time_ms, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetProtectionThresholds(addr, save_to_flash, over_temp_celsius, over_current_ma, detect_time_ms))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestHeartbeatTimeFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestHeartbeatTime(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetHeartbeatTimeFrame(uint8_t addr, bool save_to_flash, uint32_t heartbeat_time_ms, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetHeartbeatTime(addr, save_to_flash, heartbeat_time_ms))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestIntegralLimitFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestIntegralLimit(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetIntegralLimitFrame(uint8_t addr, bool save_to_flash, uint32_t integral_limit, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetIntegralLimit(addr, save_to_flash, integral_limit))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildRequestCollisionReturnAngleFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_RequestCollisionReturnAngle(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetCollisionReturnAngleFrame(uint8_t addr, bool save_to_flash, uint16_t angle_0p1deg, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetCollisionReturnAngle(addr, save_to_flash, angle_0p1deg))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildBroadcastRequestAddressFrame(uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_BroadcastRequestAddress())
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildSetParameterLockLevelFrame(uint8_t addr, bool save_to_flash, uint8_t lock_level, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_SetParameterLockLevel(addr, save_to_flash, lock_level))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmSetAutoRunFrame(uint8_t addr, const ZDT_EmmAutoRunConfig *config, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmSetAutoRun(addr, config))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmRequestAllSystemStatusFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmRequestAllSystemStatus(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmRequestDriveConfigFrame(uint8_t addr, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmRequestDriveConfig(addr))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}

zdt_cmd_status_t ZDT_BuildEmmSetDriveConfigFrame(uint8_t addr, const ZDT_EmmDriveConfig *config, uint8_t *frame, uint16_t *frame_size)
{
    if (false == zdt_begin_capture_frame(frame, frame_size))
    {
        return ZDT_CMD_STATUS_PARAMETER;
    }

    if (false == ZDT_EmmSetDriveConfig(addr, config))
    {
        zdt_end_capture_frame();
        return ZDT_CMD_STATUS_PARAMETER;
    }

    zdt_end_capture_frame();
    return ZDT_CMD_STATUS_OK;
}


