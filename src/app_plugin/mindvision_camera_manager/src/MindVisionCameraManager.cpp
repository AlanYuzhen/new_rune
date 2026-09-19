#include "MindVisionCameraManager.hpp"

#include <chrono>
#include <stdexcept>

#include <glog/logging.h>

#include "class_loader.hpp"
#include "json.hpp"
#include "time/time.hpp"

namespace app_plugin
{

MindVisionCameraManager::MindVisionCameraManager() = default;

MindVisionCameraManager::~MindVisionCameraManager()
{
    m_quit = true;
    if (m_capture_thread.joinable())
        m_capture_thread.join();
    close_camera();
}

void MindVisionCameraManager::load_config_and_open_camera()
{
    J_MV_CAMERA.updateJson();
    if (!J_MV_CAMERA.config_.isOpened())
        throw std::runtime_error("failed to open mindvision camera config");

    const cv::FileStorage &cfg = J_MV_CAMERA.config_;
    m_exposure_ms = static_cast<double>(cfg["exposure_ms"]);
    m_gamma = static_cast<double>(cfg["gamma"]);
    m_frame_speed = static_cast<int>(cfg["frame_speed"]);
    m_expected_width = static_cast<int>(cfg["expected_width"]);
    m_expected_height = static_cast<int>(cfg["expected_height"]);

    // 枚举并打开
    CameraSdkInit(1);

    int camera_num = 1;
    tSdkCameraDevInfo camera_info_list;
    tSdkCameraCapbility cap;
    CameraEnumerateDevice(&camera_info_list, &camera_num);
    if (camera_num == 0)
        throw std::runtime_error("MindVision: no camera found");

    if (CameraInit(&camera_info_list, -1, -1, &m_handle) != CAMERA_STATUS_SUCCESS)
        throw std::runtime_error("MindVision: CameraInit failed");

    CameraGetCapability(m_handle, &cap);
    m_width = cap.sResolutionRange.iWidthMax;
    m_height = cap.sResolutionRange.iHeightMax;

    CameraSetAeState(m_handle, FALSE);
    CameraSetExposureTime(m_handle, static_cast<double>(m_exposure_ms * 1e3));
    CameraSetGamma(m_handle, static_cast<int>(m_gamma * 1e2));
    CameraSetIspOutFormat(m_handle, CAMERA_MEDIA_TYPE_BGR8);
    CameraSetTriggerMode(m_handle, 0);   // 连续采集
    CameraSetFrameSpeed(m_handle, m_frame_speed);
    CameraPlay(m_handle);

    LOG(INFO) << "[MindVisionCamera] opened: " << m_width << "x" << m_height
              << " exposure=" << m_exposure_ms << "ms gamma=" << m_gamma;

    m_capturing = true;
    m_capture_thread = std::thread(&MindVisionCameraManager::capture_loop, this);
    m_initialized = true;
}

void MindVisionCameraManager::capture_loop()
{
    LOG(INFO) << "[MindVisionCamera] capture_loop started";
    tSdkFrameHead head;
    BYTE *raw = nullptr;

    while (!m_quit && m_capturing)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (CameraGetImageBuffer(m_handle, &head, &raw, 100) != CAMERA_STATUS_SUCCESS)
        {
            LOG(WARNING) << "[MindVisionCamera] GetImageBuffer failed";
            continue;
        }

        cv::Mat img(m_height, m_width, CV_8UC3);
        CameraImageProcess(m_handle, raw, img.data, &head);
        CameraReleaseImageBuffer(m_handle, raw);

        {
            std::lock_guard<std::mutex> lk(m_frame_mutex);
            m_latest_frame = std::move(img);
            m_frame_ready = true;
        }
    }
    m_capturing = false;
    LOG(INFO) << "[MindVisionCamera] capture_loop stopped";
}

void MindVisionCameraManager::close_camera()
{
    if (m_handle != -1)
    {
        CameraUnInit(m_handle);
        m_handle = -1;
    }
}

void MindVisionCameraManager::initialize_endpoints(const app::Context &context)
{
    if (!m_ecs_input)
        m_ecs_input.emplace(context.get_channel_subscriber<ECSData>(this, {}, true));
    if (!m_output)
        m_output.emplace(context.get_buffer_publisher<InputFrame>(this));
}

void MindVisionCameraManager::process(const app::Context &context)
{
    if (!m_initialized)
        load_config_and_open_camera();
    initialize_endpoints(context);

    cv::Mat frame;
    {
        std::lock_guard<std::mutex> lk(m_frame_mutex);
        if (!m_frame_ready)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return;
        }
        frame = m_latest_frame.clone();
    }

    if ((m_expected_width > 0 && frame.cols != m_expected_width) ||
        (m_expected_height > 0 && frame.rows != m_expected_height))
    {
        LOG_EVERY_N(WARNING, 100) << "[MindVisionCamera] frame "
                                   << frame.cols << "x" << frame.rows
                                   << ", expected " << m_expected_width
                                   << "x" << m_expected_height;
    }

    const std::shared_ptr<const ECSData> ecs = m_ecs_input->wait_next();

    InputFrame out;
    out.ecs_data = *ecs;
    out.timestamp = timetool::now();
    out.img = std::move(frame);
    m_output->push(std::move(out));
}

} // namespace app_plugin

REGISTER_PLUGIN("CameraManager", app_plugin::MindVisionCameraManager)
