
#include "crc_utils.h"

uint32_t reflect(uint32_t val)
{
    uint32_t result = 0;

    for (int i = 0; i < 32; i++) {
        if ((val & (1 << i)) != 0)
            result |= (1 << (31 - i));
    }

    return result;
}

uint8_t reflect8(uint8_t val)
{
    uint8_t result = 0;

    for (int i = 0; i < 8; i++) {
        if ((val & (1 << i)) != 0)
            result |= (1 << (7 - i));
    }

    return result;
}

uint32_t crc32(QByteArray data, uint32_t crc)
{
    // char *byte;

    // const uint32_t polynomial = 0x04C11DB7;

    // crc = crc ^ 0xFFFFFFFF;
    // crc = reflect(crc);

    // for(byte = data.begin(); byte < data.end(); byte++) {
    //     crc ^= ((uint32_t) reflect8(*byte)) << 24;

    //     for (int i = 0; i < 8; i++) {
    //         if ((crc & 0x80000000) != 0)
    //             crc = ((crc << 1) ^ polynomial);
    //         else
    //             crc = crc << 1;
    //     }
    // }

    // crc = crc ^ 0xFFFFFFFF;
    // return reflect(crc);

    crc = ~crc;
    char * byte;

    for(byte = data.begin(); byte < data.end(); byte++)
        crc = CRC_TABLE[(crc ^ *byte) & 0xFF] ^ (crc >> 8);

    return ~crc;
}


