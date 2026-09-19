#include "SerialSendEncoder.hpp"

#include <cmath>
#include <stdexcept>

#include <glog/logging.h>

#include "class_loader.hpp"
#include "communication/crc.hpp"
#include "json.hpp"

namespace
{
constexpr double RAD_PER_DEG = 3.14159265358979323846 / 180.0;
}

namespace app_plugin
{

SerialSendEncoder::SerialSendEncoder() = default;

SerialSendEncoder::~SerialSendEncoder()
{
    try {
        if (m_serial.isOpen()) m_serial.close();
    } catch (...) {}
}

void SerialSendEncoder::load_config()
{
    J_SEND_ENCODER.updateJson();
    if (!J_SEND_ENCODER.config_.isOpened())
        throw std::runtime_error("failed to open send encoder config");

    const cv::FileStorage &cfg = J_SEND_ENCODER.config_;
    m_port = static_cast<std::string>(cfg["serial_port"]);
    m_baudrate = static_cast<int>(cfg["baudrate"]);
    m_deg_to_rad = static_cast<int>(cfg["deg_to_rad"]) != 0;
    m_send_enabled = static_cast<int>(cfg["send_enabled"]) != 0;

    m_config_loaded = true;
    LOG(INFO) << "[SerialSendEncoder] config: port=" << m_port
              << " baud=" << m_baudrate
              << " send_enabled=" << m_send_enabled;
}

void SerialSendEncoder::open_serial()
{
    try {
        m_serial.setPort(m_port);
        m_serial.setBaudrate(static_cast<uint32_t>(m_baudrate));
        serial::Timeout to = serial::Timeout::simpleTimeout(100);
        m_serial.setTimeout(to);
        m_serial.open();
        LOG(INFO) << "[SerialSendEncoder] serial opened: " << m_port;
    } catch (const std::exception &e) {
        LOG(ERROR) << "[SerialSendEncoder] open serial failed: " << e.what();
        throw;
    }
}

communication::Command SerialSendEncoder::to_command(const FireResult &r) const
{
    communication::Command cmd;
    // 命令模块：根据火控结果决定是否控制/开火
    cmd.control = r.is_find_target || r.is_enable_fire || r.is_keep_shooting;
    cmd.shoot   = r.is_enable_fire;
    double yaw = r.yaw;
    double pitch = r.pitch;
    if (m_deg_to_rad)
    {
        yaw *= RAD_PER_DEG;
        pitch *= RAD_PER_DEG;
    }
    cmd.yaw = yaw;
    cmd.pitch = pitch;
    return cmd;
}

void SerialSendEncoder::send_command(const communication::Command &cmd)
{
    communication::VisionToGimbal frame;
    frame.mode = cmd.control ? (cmd.shoot ? 2 : 1) : 0;
    frame.yaw = static_cast<float>(cmd.yaw);
    frame.yaw_vel = static_cast<float>(cmd.yaw_vel);
    frame.yaw_acc = static_cast<float>(cmd.yaw_acc);
    frame.pitch = static_cast<float>(cmd.pitch);
    frame.pitch_vel = static_cast<float>(cmd.pitch_vel);
    frame.pitch_acc = static_cast<float>(cmd.pitch_acc);
    frame.center_yaw = static_cast<float>(cmd.center_yaw);
    frame.crc16 = communication::get_crc16(
        reinterpret_cast<uint8_t*>(&frame), sizeof(frame) - sizeof(frame.crc16));

    if (!m_send_enabled) return;

    std::lock_guard<std::mutex> lk(m_serial_mutex);
    try {
        if (m_serial.isOpen())
            m_serial.write(reinterpret_cast<uint8_t*>(&frame), sizeof(frame));
    } catch (const std::exception &e) {
        LOG(WARNING) << "[SerialSendEncoder] write failed: " << e.what();
    }
}

void SerialSendEncoder::process(const app::Context &context)
{
    if (!m_config_loaded)
    {
        load_config();
        open_serial();
    }

    if (!m_input)
        m_input.emplace(context.get_buffer_subscriber<FireResult>(this));

    const FireResult result = m_input->wait_pop();
    const bool finite = std::isfinite(result.yaw) && std::isfinite(result.pitch);
    if (!finite) {
        LOG(ERROR) << "[SerialSendEncoder] non-finite result, drop: yaw="
                   << result.yaw << " pitch=" << result.pitch;
        return;
    }

    send_command(to_command(result));
}

} // namespace app_plugin

REGISTER_PLUGIN("SendEncoder", app_plugin::SerialSendEncoder)
