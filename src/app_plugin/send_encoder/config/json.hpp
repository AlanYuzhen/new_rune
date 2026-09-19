#pragma once

#include <filesystem>

#include "json/ReJson.hpp"

inline const std::filesystem::path SEND_ENCODER_CONFIG_DIR =
    std::filesystem::path(__FILE__).parent_path();
inline ReJson J_SEND_ENCODER(
    (SEND_ENCODER_CONFIG_DIR / "serial_send.json").string());
