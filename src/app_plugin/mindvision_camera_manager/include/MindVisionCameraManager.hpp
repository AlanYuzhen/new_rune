#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <opencv2/opencv.hpp>

#include "CameraManager.hpp"
#include "CameraApi.h"

namespace app_plugin
{

// 仿照 sp_vision_25/io/mindvision 的迈德威视工业相机插件：
// 连续采集线程抓帧(SDK 直接输出 BGR)，process() 取最新帧与 ECSData 打包。
class MindVisionCameraManager final : public CameraManager
{
public:
    MindVisionCameraManager();
    ~MindVisionCameraManager() override;

    void process(const app::Context &context) override;

private:
    using EcsSubscriber = LatestChannel<ECSData>::Subscriber;
    using FramePublisher = LatestBuffer<InputFrame>::Publisher;

    void load_config_and_open_camera();
    void initialize_endpoints(const app::Context &context);

    void capture_loop();
    void close_camera();

    std::optional<EcsSubscriber> m_ecs_input;
    std::optional<FramePublisher> m_output;

    CameraHandle m_handle = -1;
    int m_width = 0;
    int m_height = 0;
    std::thread m_capture_thread;
    std::atomic<bool> m_quit{false};
    std::atomic<bool> m_capturing{false};

    std::mutex m_frame_mutex;
    cv::Mat m_latest_frame;
    bool m_frame_ready = false;

    // 配置
    double m_exposure_ms = 5.0;
    double m_gamma = 1.0;
    int m_frame_speed = 1;   // 0:低 1:中 2:高
    int m_expected_width = 0;
    int m_expected_height = 0;

    bool m_initialized = false;
};

} // namespace app_plugin
