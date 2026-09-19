#include "communication/crc.hpp"

#include "communication/crc_data.h"

namespace communication
{

uint16_t get_crc16(const uint8_t * data, uint32_t len)
{
    uint16_t crc16 = CRC16_INIT;
    uint8_t byte;
    uint8_t i;

    while (len--) {
        byte = *data++;
        i = (crc16 ^ byte) & 0x00ff;
        crc16 = (crc16 >> 8) ^ CRC16_TABLE[i];
    }
    return crc16;
}

bool check_crc16(const uint8_t * data, uint32_t len)
{
    uint16_t crc16 = (data[len - 1] << 8) | data[len - 2];
    return get_crc16(data, len - 2) == crc16;
}

} // namespace communication
