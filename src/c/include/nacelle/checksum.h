#ifndef NACELLE_CHECKSUM_H
#define NACELLE_CHECKSUM_H

#include "nacelle/types.h"

#define NACELLE_CRC16_POLY 0x1021U
#define NACELLE_CRC16_INIT 0xFFFFU

uint16_t nacelle_crc16_step_bit(uint16_t crc, bool bit);
uint16_t nacelle_crc16_step_byte(uint16_t crc, uint8_t byte);
uint16_t nacelle_crc16_step_fast(uint16_t crc, uint8_t byte);
uint16_t nacelle_crc16_compute(const uint8_t *data, size_t length);
bool nacelle_crc16_verify(const uint8_t *data, size_t length, uint16_t expected_crc);

#endif /* NACELLE_CHECKSUM_H */
