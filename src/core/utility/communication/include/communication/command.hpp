#pragma once

namespace communication
{

// 视觉决策层 -> 云台发送层 的控制命令
// 仿照 sp_vision_25/io/command.hpp，作为 FireResult 与串口协议帧之间的中间表示
struct Command
{
    bool control = false;      // 是否控制云台
    bool shoot   = false;      // 是否开火
    double yaw   = 0.0;        // 目标 yaw (rad)
    double pitch = 0.0;        // 目标 pitch (rad)
    double yaw_vel   = 0.0;
    double yaw_acc   = 0.0;
    double pitch_vel = 0.0;
    double pitch_acc = 0.0;
    double center_yaw = 0.0;   // 大符旋转中心 yaw (rad)
};

} // namespace communication
