#pragma once

#include <filesystem>

#include "json/ReJson.hpp"

inline const std::filesystem::path MV_CAMERA_CONFIG_DIR =
    std::filesystem::path(__FILE__).parent_path();
inline ReJson J_MV_CAMERA(
    (MV_CAMERA_CONFIG_DIR / "mv_camera.json").string());
