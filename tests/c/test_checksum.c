#include "nacelle/checksum.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_crc16_standard_vector(void) {
    const char *data = "123456789";
    uint16_t crc = nacelle_crc16_compute((const uint8_t *)data, 9);
    assert(crc == 0x29B1);
    assert(nacelle_crc16_verify((const uint8_t *)data, 9, 0x29B1));
    assert(!nacelle_crc16_verify((const uint8_t *)data, 9, 0x0000));
}

static void test_crc16_bit_step_equivalence(void) {
    uint8_t sample[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0xAA, 0x55, 0xFF, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};

    uint16_t crc_fast = nacelle_crc16_compute(sample, sizeof(sample));

    uint16_t crc_bit = NACELLE_CRC16_INIT;
    for (size_t i = 0; i < sizeof(sample); ++i) {
        crc_bit = nacelle_crc16_step_byte(crc_bit, sample[i]);
    }

    assert(crc_fast == crc_bit);
}

static void test_crc16_single_bit_flip_sensitivity(void) {
    uint8_t orig[32];
    memset(orig, 0x3C, sizeof(orig));
    uint16_t crc_orig = nacelle_crc16_compute(orig, sizeof(orig));

    for (size_t byte_idx = 0; byte_idx < sizeof(orig); ++byte_idx) {
        for (int bit_idx = 0; bit_idx < 8; ++bit_idx) {
            uint8_t corrupted[32];
            memcpy(corrupted, orig, sizeof(orig));
            corrupted[byte_idx] ^= (uint8_t)(1U << bit_idx);

            uint16_t crc_corrupt = nacelle_crc16_compute(corrupted, sizeof(corrupted));
            assert(crc_corrupt != crc_orig);
        }
    }
}

int main(void) {
    printf("[TEST] Running checksum unit tests...\n");
    test_crc16_standard_vector();
    test_crc16_bit_step_equivalence();
    test_crc16_single_bit_flip_sensitivity();
    printf("[PASS] All checksum unit tests passed.\n");
    return 0;
}
