# 公共通信库：串口(serial) + 云台协议帧 + CRC16 + 命令结构
# 供 SerialReceiveDecoder / SerialSendEncoder 两个插件链接

set(COMMUNICATION_TARGET communication_lib)

set(SERIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/serial)

set(SERIAL_SRCS
    ${SERIAL_DIR}/src/serial.cc
    ${SERIAL_DIR}/src/impl/unix.cc
    ${SERIAL_DIR}/src/impl/list_ports/list_ports_linux.cc
)

add_library(${COMMUNICATION_TARGET} STATIC
    ${SERIAL_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crc.cpp
)

# 静态库会被链接进插件 .so，必须开 PIC
set_target_properties(${COMMUNICATION_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(${COMMUNICATION_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SERIAL_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(${COMMUNICATION_TARGET} PUBLIC
    Threads::Threads
    rt
)
