#pragma once

#include <cstdint>

namespace communication
{

// 下位机 -> 视觉 (Gimbal -> Vision)
// 与 sp_vision_25/io/gimbal 的 GimbalToVision 保持字节级一致
struct __attribute__((packed)) GimbalToVision
{
    uint8_t head[2] = {'S', 'P'};
    uint8_t mode;            // 0:空闲 1:自瞄 2:小符 3:大符
    float q[4];              // 四元数 wxyz
    float yaw;               // 弧度
    float yaw_vel;           // rad/s
    float pitch;             // 弧度
    float pitch_vel;         // rad/s
    float bullet_speed;      // m/s
    uint16_t bullet_count;   // 子弹累计计数
    uint16_t crc16;
};

static_assert(sizeof(GimbalToVision) <= 64, "GimbalToVision too large");

// 视觉 -> 下位机 (Vision -> Gimbal)
// 与 sp_vision_25/io/gimbal 的 VisionToGimbal 保持字节级一致
struct __attribute__((packed)) VisionToGimbal
{
    uint8_t head[2] = {'S', 'P'};
    uint8_t mode;            // 0:不控制 1:控制云台不开火 2:控制云台且开火
    float yaw;               // 弧度
    float yaw_vel;           // rad/s
    float yaw_acc;           // rad/s^2
    float pitch;             // 弧度
    float pitch_vel;         // rad/s
    float pitch_acc;         // rad/s^2
    float center_yaw;        // 大符旋转中心 yaw (rad)
    uint16_t crc16;
};

static_assert(sizeof(VisionToGimbal) <= 64, "VisionToGimbal too large");

// 协议里的工作模式枚举，与下位机约定
enum class GimbalMode : uint8_t
{
    IDLE      = 0,
    AUTO_AIM  = 1,
    SMALL_RUNE = 2,
    BIG_RUNE  = 3
};

} // namespace communication
