# send_encoder：同时编译仿真版与真实串口版两个独立动态库

# ---- 仿真版（保持原文件名）----
set(SEND_ENCODER_PLUGIN_TARGET send_encoder_plugin_lib)
add_library(${SEND_ENCODER_PLUGIN_TARGET} SHARED
    src/SimulatedSendEncoder.cpp
)
target_include_directories(${SEND_ENCODER_PLUGIN_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${SEND_ENCODER_PLUGIN_TARGET} PUBLIC
    app_interface_lib
    class_loader_lib
    google_logger_lib
)

# ---- 真实串口版 ----
set(SEND_ENCODER_SERIAL_TARGET send_encoder_serial_plugin_lib)
add_library(${SEND_ENCODER_SERIAL_TARGET} SHARED
    src/SerialSendEncoder.cpp
)
target_include_directories(${SEND_ENCODER_SERIAL_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)
target_link_libraries(${SEND_ENCODER_SERIAL_TARGET} PUBLIC
    app_interface_lib
    class_loader_lib
    config_loader_lib
    google_logger_lib
    communication_lib
)
