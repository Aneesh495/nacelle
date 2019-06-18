#include "nacelle/ring_buffer.h"
#include <string.h>

void nacelle_ring_buffer_init(nacelle_ring_buffer_t *rb) {
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->total_written = 0;
    rb->total_overruns = 0;
}

size_t nacelle_ring_buffer_available(const nacelle_ring_buffer_t *rb) {
    return rb ? rb->count : 0;
}

size_t nacelle_ring_buffer_free_space(const nacelle_ring_buffer_t *rb) {
    return rb ? (NACELLE_RING_BUFFER_CAPACITY - rb->count) : 0;
}

nacelle_status_t nacelle_ring_buffer_write(nacelle_ring_buffer_t *rb, const uint8_t *data, size_t length) {
    if (!rb || !data) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (length == 0) {
        return NACELLE_OK;
    }
    if (length > nacelle_ring_buffer_free_space(rb)) {
        rb->total_overruns++;
        return NACELLE_ERR_BUFFER_OVERFLOW;
    }

    for (size_t i = 0; i < length; ++i) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % NACELLE_RING_BUFFER_CAPACITY;
    }

    rb->count += length;
    rb->total_written += (uint32_t)length;
    return NACELLE_OK;
}

nacelle_status_t nacelle_ring_buffer_read(nacelle_ring_buffer_t *rb, uint8_t *out_data, size_t length, size_t *out_read) {
    if (!rb || !out_data) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    size_t to_read = (length < rb->count) ? length : rb->count;
    for (size_t i = 0; i < to_read; ++i) {
        out_data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % NACELLE_RING_BUFFER_CAPACITY;
    }

    rb->count -= to_read;
    if (out_read) {
        *out_read = to_read;
    }

    return (to_read == length) ? NACELLE_OK : NACELLE_ERR_BUFFER_UNDERFLOW;
}

nacelle_status_t nacelle_ring_buffer_peek(const nacelle_ring_buffer_t *rb, uint8_t *out_data, size_t length, size_t *out_read) {
    if (!rb || !out_data) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    size_t to_read = (length < rb->count) ? length : rb->count;
    size_t temp_tail = rb->tail;
    for (size_t i = 0; i < to_read; ++i) {
        out_data[i] = rb->buffer[temp_tail];
        temp_tail = (temp_tail + 1) % NACELLE_RING_BUFFER_CAPACITY;
    }

    if (out_read) {
        *out_read = to_read;
    }

    return (to_read == length) ? NACELLE_OK : NACELLE_ERR_BUFFER_UNDERFLOW;
}

nacelle_status_t nacelle_ring_buffer_drop(nacelle_ring_buffer_t *rb, size_t length) {
    if (!rb) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    size_t to_drop = (length < rb->count) ? length : rb->count;
    rb->tail = (rb->tail + to_drop) % NACELLE_RING_BUFFER_CAPACITY;
    rb->count -= to_drop;

    return (to_drop == length) ? NACELLE_OK : NACELLE_ERR_BUFFER_UNDERFLOW;
}
