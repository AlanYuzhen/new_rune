#pragma once

#include <filesystem>

#include "json/ReJson.hpp"

inline const std::filesystem::path HIK_CAMERA_CONFIG_DIR =
    std::filesystem::path(__FILE__).parent_path();
inline ReJson J_HIK_CAMERA(
    (HIK_CAMERA_CONFIG_DIR / "hik_camera.json").string());
