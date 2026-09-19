#pragma once

#include <filesystem>

#include "json/ReJson.hpp"

inline const std::filesystem::path RECEIVE_DECODER_CONFIG_DIR =
    std::filesystem::path(__FILE__).parent_path();
inline ReJson J_RECEIVE_DECODER(
    (RECEIVE_DECODER_CONFIG_DIR / "receive_decoder.json").string());

// 真实串口接收插件使用的配置
inline ReJson J_SERIAL_RECEIVE(
    (RECEIVE_DECODER_CONFIG_DIR / "serial_receive.json").string());
