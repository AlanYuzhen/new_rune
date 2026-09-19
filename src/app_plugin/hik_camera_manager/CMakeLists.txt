# 真实海康工业相机插件

set(HIK_CAMERA_TARGET hik_camera_manager_plugin_lib)

find_package(OpenCV REQUIRED COMPONENTS core imgproc)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

set(HIK_SDK_DIR ${CMAKE_SOURCE_DIR}/third_party/hikrobot)
set(HIK_SDK_LIB ${HIK_SDK_DIR}/lib/amd64/libMvCameraControl.so)

add_library(${HIK_CAMERA_TARGET} SHARED
    src/HikCameraManager.cpp
)

target_include_directories(${HIK_CAMERA_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${HIK_SDK_DIR}/include
    ${LIBUSB_INCLUDE_DIRS}
)

target_link_libraries(${HIK_CAMERA_TARGET} PUBLIC
    app_interface_lib
    class_loader_lib
    config_loader_lib
    google_logger_lib
    opencv_core
    opencv_imgproc
    ${HIK_SDK_LIB}
    ${LIBUSB_LIBRARIES}
)
