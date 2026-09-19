#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <opencv2/opencv.hpp>

#include "CameraManager.hpp"

namespace app_plugin
{

// 仿照 sp_vision_25/io/hikrobot 的海康工业相机读取插件：
// 独立采集线程抓帧(Bayer->BGR)，process() 取最新帧并与最新 ECSData 打包为 InputFrame。
class HikCameraManager final : public CameraManager
{
public:
    HikCameraManager();
    ~HikCameraManager() override;

    void process(const app::Context &context) override;

private:
    using EcsSubscriber = LatestChannel<ECSData>::Subscriber;
    using FramePublisher = LatestBuffer<InputFrame>::Publisher;

    void load_config_and_open_camera();
    void initialize_endpoints(const app::Context &context);

    void capture_loop();
    void stop_grab();

    void set_float(const char *name, double value);
    void set_enum(const char *name, unsigned int value);

    std::optional<EcsSubscriber> m_ecs_input;
    std::optional<FramePublisher> m_output;

    void *m_handle = nullptr;
    std::thread m_capture_thread;
    std::atomic<bool> m_quit{false};
    std::atomic<bool> m_grabbing{false};

    std::mutex m_frame_mutex;
    cv::Mat m_latest_frame;
    std::chrono::steady_clock::time_point m_latest_ts{};
    bool m_frame_ready = false;

    // 配置
    double m_exposure_ms = 5.0;
    double m_gain = 0.0;
    std::string m_serial;
    int m_expected_width = 0;
    int m_expected_height = 0;
    double m_target_fps = 100.0;

    bool m_initialized = false;
};

} // namespace app_plugin
