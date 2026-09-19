# 迈德威视(MindVision)工业相机插件

set(MV_CAMERA_TARGET mindvision_camera_manager_plugin_lib)

find_package(OpenCV REQUIRED COMPONENTS core imgproc)

set(MV_SDK_DIR ${CMAKE_SOURCE_DIR}/third_party/mindvision)
set(MV_SDK_LIB ${MV_SDK_DIR}/lib/amd64/libMVSDK.so)

add_library(${MV_CAMERA_TARGET} SHARED
    src/MindVisionCameraManager.cpp
)

target_include_directories(${MV_CAMERA_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${MV_SDK_DIR}/include
)

target_link_libraries(${MV_CAMERA_TARGET} PUBLIC
    app_interface_lib
    class_loader_lib
    config_loader_lib
    google_logger_lib
    opencv_core
    opencv_imgproc
    ${MV_SDK_LIB}
)
