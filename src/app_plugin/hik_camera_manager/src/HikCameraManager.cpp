#include "HikCameraManager.hpp"

#include <chrono>
#include <stdexcept>
#include <unordered_map>

#include <glog/logging.h>
#include <MvCameraControl.h>
#include <libusb-1.0/libusb.h>

#include "class_loader.hpp"
#include "json.hpp"
#include "time/time.hpp"

namespace
{
std::unordered_map<MvGvspPixelType, cv::ColorConversionCodes> make_bayer_map()
{
    return {
        {PixelType_Gvsp_BayerGR8, cv::COLOR_BayerGR2BGR},
        {PixelType_Gvsp_BayerRG8, cv::COLOR_BayerRG2BGR},
        {PixelType_Gvsp_BayerGB8, cv::COLOR_BayerGB2BGR},
        {PixelType_Gvsp_BayerBG8, cv::COLOR_BayerBG2BGR},
    };
}
} // namespace

namespace app_plugin
{

HikCameraManager::HikCameraManager()
{
    // sp_vision_25 同款：海康 SDK 枚举 USB3 相机前必须先初始化 libusb
    if (libusb_init(nullptr))
        LOG(WARNING) << "[HikCamera] libusb_init failed";
}

HikCameraManager::~HikCameraManager()
{
    m_quit = true;
    if (m_capture_thread.joinable())
        m_capture_thread.join();
    stop_grab();
}

void HikCameraManager::load_config_and_open_camera()
{
    J_HIK_CAMERA.updateJson();
    if (!J_HIK_CAMERA.config_.isOpened())
        throw std::runtime_error("failed to open hik camera config");

    const cv::FileStorage &cfg = J_HIK_CAMERA.config_;
    m_exposure_ms = static_cast<double>(cfg["exposure_ms"]);
    m_gain = static_cast<double>(cfg["gain"]);
    m_serial = static_cast<std::string>(cfg["serial_number"]);
    m_expected_width = static_cast<int>(cfg["expected_width"]);
    m_expected_height = static_cast<int>(cfg["expected_height"]);
    m_target_fps = static_cast<double>(cfg["fps"]);
    if (m_target_fps <= 0.0) m_target_fps = 100.0;

    // 枚举设备
    MV_CC_DEVICE_INFO_LIST device_list;
    unsigned int ret = MV_CC_EnumDevices(MV_USB_DEVICE, &device_list);
    if (ret != MV_OK)
        throw std::runtime_error("MV_CC_EnumDevices failed: " + std::to_string(ret));
    if (device_list.nDeviceNum == 0)
        throw std::runtime_error("no HikRobot camera found");

    int target = 0;
    for (unsigned int i = 0; i < device_list.nDeviceNum; ++i)
    {
        auto *info = device_list.pDeviceInfo[i];
        std::string sn = (info->nTLayerType == MV_USB_DEVICE)
            ? reinterpret_cast<const char*>(info->SpecialInfo.stUsb3VInfo.chSerialNumber)
            : "";
        LOG(INFO) << "[HikCamera] enum [" << i << "/" << device_list.nDeviceNum-1
                  << "] sn=" << sn;
        if (!m_serial.empty() && sn == m_serial)
            target = i;
    }

    ret = MV_CC_CreateHandle(&m_handle, device_list.pDeviceInfo[target]);
    if (ret != MV_OK)
        throw std::runtime_error("MV_CC_CreateHandle failed: " + std::to_string(ret));

    ret = MV_CC_OpenDevice(m_handle);
    if (ret != MV_OK)
        throw std::runtime_error("MV_CC_OpenDevice failed: " + std::to_string(ret));

    set_enum("BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_CONTINUOUS);
    set_enum("ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
    set_enum("GainAuto", MV_GAIN_MODE_OFF);
    set_float("ExposureTime", m_exposure_ms * 1e3);
    set_float("Gain", m_gain);
    MV_CC_SetFrameRate(m_handle, static_cast<float>(m_target_fps));

    LOG(INFO) << "[HikCamera] opened: exposure=" << m_exposure_ms
              << "ms gain=" << m_gain << " fps=" << m_target_fps;

    ret = MV_CC_StartGrabbing(m_handle);
    if (ret != MV_OK)
        throw std::runtime_error("MV_CC_StartGrabbing failed: " + std::to_string(ret));

    m_grabbing = true;
    m_capture_thread = std::thread(&HikCameraManager::capture_loop, this);
    m_initialized = true;
}

void HikCameraManager::capture_loop()
{
    static const auto bayer_map = make_bayer_map();
    LOG(INFO) << "[HikCamera] capture_loop started";

    MV_FRAME_OUT raw;
    while (!m_quit && m_grabbing)
    {
        unsigned int ret = MV_CC_GetImageBuffer(m_handle, &raw, 200);
        if (ret != MV_OK)
        {
            LOG(WARNING) << "[HikCamera] GetImageBuffer failed: " << std::hex << ret;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const auto &fi = raw.stFrameInfo;
        cv::Mat gray(cv::Size(fi.nWidth, fi.nHeight), CV_8U, raw.pBufAddr);

        cv::Mat bgr;
        auto it = bayer_map.find(fi.enPixelType);
        if (it != bayer_map.end())
            cv::cvtColor(gray, bgr, it->second);
        else
        {
            // 已是 BGR/灰度则直接拷贝
            bgr = gray.clone();
        }

        {
            std::lock_guard<std::mutex> lk(m_frame_mutex);
            m_latest_frame = bgr.clone();
            m_latest_ts = std::chrono::steady_clock::now();
            m_frame_ready = true;
        }

        MV_CC_FreeImageBuffer(m_handle, &raw);
    }
    m_grabbing = false;
    LOG(INFO) << "[HikCamera] capture_loop stopped";
}

void HikCameraManager::stop_grab()
{
    if (m_handle)
    {
        if (m_grabbing) MV_CC_StopGrabbing(m_handle);
        MV_CC_CloseDevice(m_handle);
        MV_CC_DestroyHandle(m_handle);
        m_handle = nullptr;
    }
}

void HikCameraManager::set_float(const char *name, double value)
{
    unsigned int ret = MV_CC_SetFloatValue(m_handle, name, static_cast<float>(value));
    if (ret != MV_OK)
        LOG(WARNING) << "[HikCamera] SetFloatValue(" << name << ") failed: " << std::hex << ret;
}

void HikCameraManager::set_enum(const char *name, unsigned int value)
{
    unsigned int ret = MV_CC_SetEnumValue(m_handle, name, value);
    if (ret != MV_OK)
        LOG(WARNING) << "[HikCamera] SetEnum(" << name << ") failed: " << std::hex << ret;
}

void HikCameraManager::initialize_endpoints(const app::Context &context)
{
    if (!m_ecs_input)
        m_ecs_input.emplace(context.get_channel_subscriber<ECSData>(this, {}, true));
    if (!m_output)
        m_output.emplace(context.get_buffer_publisher<InputFrame>(this));
}

void HikCameraManager::process(const app::Context &context)
{
    if (!m_initialized)
        load_config_and_open_camera();
    initialize_endpoints(context);

    cv::Mat frame;
    std::chrono::steady_clock::time_point ts;
    {
        std::lock_guard<std::mutex> lk(m_frame_mutex);
        if (!m_frame_ready)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return;
        }
        frame = m_latest_frame.clone();
        ts = m_latest_ts;
    }

    if ((m_expected_width > 0 && frame.cols != m_expected_width) ||
        (m_expected_height > 0 && frame.rows != m_expected_height))
    {
        LOG_EVERY_N(WARNING, 100) << "[HikCamera] frame size "
                                   << frame.cols << "x" << frame.rows
                                   << ", expected " << m_expected_width << "x" << m_expected_height;
    }

    const std::shared_ptr<const ECSData> ecs = m_ecs_input->wait_next();

    InputFrame out;
    out.ecs_data = *ecs;
    out.timestamp = timetool::now();
    out.img = std::move(frame);
    m_output->push(std::move(out));
}

} // namespace app_plugin

REGISTER_PLUGIN("CameraManager", app_plugin::HikCameraManager)
