#include "SerialReceiveDecoder.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

#include <glog/logging.h>

#include "class_loader.hpp"
#include "communication/crc.hpp"
#include "json.hpp"

namespace
{
constexpr double DEG_PER_RAD = 180.0 / 3.14159265358979323846;

AimMode map_mode(uint8_t mode, AimMode fallback)
{
    switch (mode)
    {
    case 1: return AimMode::AutoAim;
    case 2: return AimMode::SmallRune;
    case 3: return AimMode::BigRune;
    default: return fallback; // 0/未知 保持上次模式
    }
}
} // namespace

namespace app_plugin
{

SerialReceiveDecoder::SerialReceiveDecoder() = default;

SerialReceiveDecoder::~SerialReceiveDecoder()
{
    m_quit = true;
    if (m_read_thread.joinable())
        m_read_thread.join();
    try {
        if (m_serial.isOpen()) m_serial.close();
    } catch (...) {}
}

void SerialReceiveDecoder::load_config()
{
    J_SERIAL_RECEIVE.updateJson();
    if (!J_SERIAL_RECEIVE.config_.isOpened())
        throw std::runtime_error("failed to open serial receive config");

    const cv::FileStorage &cfg = J_SERIAL_RECEIVE.config_;
    m_port = static_cast<std::string>(cfg["serial_port"]);
    m_baudrate = static_cast<int>(cfg["baudrate"]);
    m_rad_to_deg = static_cast<int>(cfg["rad_to_deg"]) != 0;
    m_default_color = static_cast<int>(cfg["my_color"]) == static_cast<int>(MyColor::Blue)
                          ? MyColor::Blue : MyColor::Red;
    m_default_start = static_cast<int>(cfg["is_start"]) != 0;
    m_default_ready = static_cast<int>(cfg["is_ready"]) != 0;
    m_publish_hz = static_cast<double>(cfg["publish_hz"]);
    if (m_publish_hz <= 0.0) m_publish_hz = 100.0;

    // 初始 ECSData
    m_latest.my_color = m_default_color;
    m_latest.mode = AimMode::BigRune;
    m_latest.is_start = m_default_start;
    m_latest.is_ready = m_default_ready;
    m_latest.yaw = m_latest.pitch = m_latest.roll = 0.0;

    m_config_loaded = true;
    LOG(INFO) << "[SerialReceiveDecoder] config: port=" << m_port
              << " baud=" << m_baudrate
              << " rad_to_deg=" << m_rad_to_deg
              << " publish_hz=" << m_publish_hz;
}

void SerialReceiveDecoder::open_serial()
{
    try {
        m_serial.setPort(m_port);
        m_serial.setBaudrate(static_cast<uint32_t>(m_baudrate));
        serial::Timeout to = serial::Timeout::simpleTimeout(100);
        m_serial.setTimeout(to);
        m_serial.open();
        LOG(INFO) << "[SerialReceiveDecoder] serial opened: " << m_port;
    } catch (const std::exception &e) {
        LOG(ERROR) << "[SerialReceiveDecoder] open serial failed: " << e.what();
        throw;
    }
}

ECSData SerialReceiveDecoder::decode(const communication::GimbalToVision &frame) const
{
    ECSData d{};
    d.my_color = m_default_color;
    d.is_start = m_default_start;
    d.is_ready = m_default_ready;

    // 模式：保持上次作为 fallback
    d.mode = map_mode(frame.mode, m_latest.mode);

    double yaw = frame.yaw;
    double pitch = frame.pitch;
    if (m_rad_to_deg)
    {
        yaw *= DEG_PER_RAD;
        pitch *= DEG_PER_RAD;
    }
    d.yaw = yaw;
    d.pitch = pitch;
    d.roll = 0.0;
    return d;
}

void SerialReceiveDecoder::read_loop()
{
    LOG(INFO) << "[SerialReceiveDecoder] read_loop started.";

    communication::GimbalToVision frame;
    int error_count = 0;

    // 与 sp_vision_25/io/gimbal 的 read_thread 保持一致：
    // 帧头同步 -> 读剩余 -> 解析，不校验 CRC（与原版一致），错误累计 100 次后重连。
    auto read = [this](uint8_t * buffer, size_t size) -> bool {
        try {
            return m_serial.read(buffer, size) == size;
        } catch (...) {
            return false;
        }
    };

    while (!m_quit)
    {
        if (error_count > 100)
        {
            error_count = 0;
            reconnect();
            continue;
        }

        if (!read(reinterpret_cast<uint8_t*>(&frame), sizeof(frame.head))) {
            error_count++;
            continue;
        }

        if (frame.head[0] != 'S' || frame.head[1] != 'P')
            continue;

        if (!read(reinterpret_cast<uint8_t*>(&frame) + sizeof(frame.head),
                  sizeof(frame) - sizeof(frame.head))) {
            error_count++;
            continue;
        }

        error_count = 0;

        ECSData d = decode(frame);
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_latest = d;
            m_have_latest = true;
        }
    }
    LOG(INFO) << "[SerialReceiveDecoder] read_loop stopped.";
}

void SerialReceiveDecoder::reconnect()
{
    LOG(WARNING) << "[SerialReceiveDecoder] reconnecting " << m_port;
    for (int i = 0; i < 10 && !m_quit; ++i)
    {
        try {
            if (m_serial.isOpen()) m_serial.close();
        } catch (...) {}
        std::this_thread::sleep_for(std::chrono::seconds(1));
        try {
            m_serial.open();
            if (m_serial.isOpen()) {
                LOG(INFO) << "[SerialReceiveDecoder] reconnected";
                return;
            }
        } catch (const std::exception &e) {
            LOG(WARNING) << "[SerialReceiveDecoder] reconnect failed: " << e.what();
        }
    }
}

void SerialReceiveDecoder::process(const app::Context &context)
{
    if (!m_config_loaded)
    {
        load_config();
        open_serial();
        m_read_thread = std::thread(&SerialReceiveDecoder::read_loop, this);
    }

    if (!m_output)
        m_output.emplace(context.get_channel_publisher<ECSData>(this));

    ECSData d;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        d = m_latest;
    }
    m_output->publish(std::make_shared<const ECSData>(d));

    std::this_thread::sleep_for(
        std::chrono::duration<double>(1.0 / m_publish_hz));
}

} // namespace app_plugin

REGISTER_PLUGIN("ReceiveDecoder", app_plugin::SerialReceiveDecoder)
