#include "nacelle/ring_buffer.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_ring_buffer_basic_rw(void) {
    nacelle_ring_buffer_t rb;
    nacelle_ring_buffer_init(&rb);
    assert(nacelle_ring_buffer_available(&rb) == 0);
    assert(nacelle_ring_buffer_free_space(&rb) == NACELLE_RING_BUFFER_CAPACITY);

    uint8_t data_in[32];
    for (int i = 0; i < 32; ++i) data_in[i] = (uint8_t)i;

    assert(nacelle_ring_buffer_write(&rb, data_in, 32) == NACELLE_OK);
    assert(nacelle_ring_buffer_available(&rb) == 32);

    uint8_t data_out[32];
    size_t read_bytes = 0;
    assert(nacelle_ring_buffer_read(&rb, data_out, 32, &read_bytes) == NACELLE_OK);
    assert(read_bytes == 32);
    assert(memcmp(data_in, data_out, 32) == 0);
    assert(nacelle_ring_buffer_available(&rb) == 0);
}

static void test_ring_buffer_wraparound(void) {
    nacelle_ring_buffer_t rb;
    nacelle_ring_buffer_init(&rb);

    uint8_t chunk[500];
    memset(chunk, 0xAA, sizeof(chunk));

    /* Write 500, read 500 */
    assert(nacelle_ring_buffer_write(&rb, chunk, 500) == NACELLE_OK);
    uint8_t dummy[500];
    assert(nacelle_ring_buffer_read(&rb, dummy, 500, NULL) == NACELLE_OK);

    /* Write 700 bytes -> head wraps around 1024 boundary! */
    memset(chunk, 0x55, 300);
    assert(nacelle_ring_buffer_write(&rb, chunk, 300) == NACELLE_OK);
    assert(nacelle_ring_buffer_write(&rb, chunk, 400) == NACELLE_OK);
    assert(nacelle_ring_buffer_available(&rb) == 700);

    /* Read back 700 bytes and verify */
    uint8_t out[700];
    size_t read_n = 0;
    assert(nacelle_ring_buffer_read(&rb, out, 700, &read_n) == NACELLE_OK);
    assert(read_n == 700);
    assert(nacelle_ring_buffer_available(&rb) == 0);
}

static void test_ring_buffer_overflow_rejection(void) {
    nacelle_ring_buffer_t rb;
    nacelle_ring_buffer_init(&rb);

    uint8_t big[NACELLE_RING_BUFFER_CAPACITY + 1];
    memset(big, 0xFF, sizeof(big));

    assert(nacelle_ring_buffer_write(&rb, big, sizeof(big)) == NACELLE_ERR_BUFFER_OVERFLOW);
    assert(rb.total_overruns == 1);
    assert(nacelle_ring_buffer_available(&rb) == 0);
}

int main(void) {
    printf("[TEST] Running circular ring buffer unit tests...\n");
    test_ring_buffer_basic_rw();
    test_ring_buffer_wraparound();
    test_ring_buffer_overflow_rejection();
    printf("[PASS] All ring buffer unit tests passed.\n");
    return 0;
}
