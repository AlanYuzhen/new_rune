# receive_decoder：同时编译仿真版与真实串口版两个独立动态库
# runtime_config 里通过 library 字段选择

# ---- 仿真版（保持原文件名）----
set(RECEIVE_DECODER_PLUGIN_TARGET receive_decoder_plugin_lib)
add_library(${RECEIVE_DECODER_PLUGIN_TARGET} SHARED
    src/SimulatedReceiveDecoder.cpp
)
target_include_directories(${RECEIVE_DECODER_PLUGIN_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)
target_link_libraries(${RECEIVE_DECODER_PLUGIN_TARGET} PUBLIC
    app_interface_lib
    class_loader_lib
    config_loader_lib
    google_logger_lib
)

# ---- 真实串口版 ----
set(RECEIVE_DECODER_SERIAL_TARGET receive_decoder_serial_plugin_lib)
add_library(${RECEIVE_DECODER_SERIAL_TARGET} SHARED
    src/SerialReceiveDecoder.cpp
)
target_include_directories(${RECEIVE_DECODER_SERIAL_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)
target_link_libraries(${RECEIVE_DECODER_SERIAL_TARGET} PUBLIC
    app_interface_lib
    class_loader_lib
    config_loader_lib
    google_logger_lib
    communication_lib
)
