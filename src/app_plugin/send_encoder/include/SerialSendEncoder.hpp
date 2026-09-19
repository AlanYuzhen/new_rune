#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>

#include "SendEncoder.hpp"
#include "communication/command.hpp"
#include "communication/protocol.hpp"
#include "serial/serial.h"

namespace app_plugin
{

// 仿照 sp_vision_25/io/gimbal 的真实串口发送：
// 订阅 FireResult，打包为 VisionToGimbal 协议帧(SP头+CRC16)通过串口发给下位机。
// "命令模块"体现在 fire 结果 -> Command -> 协议帧 的转换。
class SerialSendEncoder final : public SendEncoder
{
public:
    SerialSendEncoder();
    ~SerialSendEncoder() override;

    void process(const app::Context &context) override;

private:
    using FireSubscriber = LatestBuffer<FireResult>::Subscriber;

    void load_config();
    void open_serial();
    communication::Command to_command(const FireResult &result) const;
    void send_command(const communication::Command &cmd);

    std::optional<FireSubscriber> m_input;

    serial::Serial m_serial;
    std::mutex m_serial_mutex;

    std::string m_port;
    int m_baudrate = 115200;
    bool m_deg_to_rad = true;      // FireResult 为角度，协议帧为弧度
    bool m_send_enabled = true;

    bool m_config_loaded = false;
};

} // namespace app_plugin
