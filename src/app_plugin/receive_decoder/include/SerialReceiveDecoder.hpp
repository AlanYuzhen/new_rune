#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "ReceiveDecoder.hpp"
#include "communication/protocol.hpp"
#include "serial/serial.h"

namespace app_plugin
{

// 仿照 sp_vision_25/io/gimbal 的真实串口接收：
// 后台线程按 "SP" 帧头同步下位机数据，CRC16 校验，解析为 ECSData 发布到 channel。
class SerialReceiveDecoder final : public ReceiveDecoder
{
public:
    SerialReceiveDecoder();
    ~SerialReceiveDecoder() override;

    void process(const app::Context &context) override;

private:
    using Publisher = LatestChannel<ECSData>::Publisher;

    void load_config();
    void open_serial();
    void read_loop();
    void reconnect();
    ECSData decode(const communication::GimbalToVision &frame) const;

    std::optional<Publisher> m_output;

    serial::Serial m_serial;
    std::thread m_read_thread;
    std::atomic<bool> m_quit{false};

    // 读线程写入、process 线程读取的最新 ECSData
    std::mutex m_mutex;
    ECSData m_latest{};
    bool m_have_latest = false;

    // 配置
    std::string m_port;
    int m_baudrate = 115200;
    bool m_rad_to_deg = true;     // 协议帧 yaw/pitch 为弧度，ECSData 用角度
    MyColor m_default_color = MyColor::Red;
    bool m_default_start = true;
    bool m_default_ready = true;
    double m_publish_hz = 100.0;

    bool m_config_loaded = false;
};

} // namespace app_plugin
