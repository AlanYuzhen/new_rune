#pragma once

#include <cstdint>

namespace communication
{

// len 不含 crc16
uint16_t get_crc16(const uint8_t * data, uint32_t len);

// len 含 crc16 (末尾两字节小端)
bool check_crc16(const uint8_t * data, uint32_t len);

} // namespace communication
