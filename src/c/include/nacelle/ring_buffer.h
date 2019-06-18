#ifndef NACELLE_RING_BUFFER_H
#define NACELLE_RING_BUFFER_H

#include "nacelle/types.h"

#define NACELLE_RING_BUFFER_CAPACITY 1024

typedef struct {
    uint8_t buffer[NACELLE_RING_BUFFER_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    uint32_t total_written;
    uint32_t total_overruns;
} nacelle_ring_buffer_t;

void nacelle_ring_buffer_init(nacelle_ring_buffer_t *rb);
size_t nacelle_ring_buffer_available(const nacelle_ring_buffer_t *rb);
size_t nacelle_ring_buffer_free_space(const nacelle_ring_buffer_t *rb);

nacelle_status_t nacelle_ring_buffer_write(nacelle_ring_buffer_t *rb, const uint8_t *data, size_t length);
nacelle_status_t nacelle_ring_buffer_read(nacelle_ring_buffer_t *rb, uint8_t *out_data, size_t length, size_t *out_read);
nacelle_status_t nacelle_ring_buffer_peek(const nacelle_ring_buffer_t *rb, uint8_t *out_data, size_t length, size_t *out_read);
nacelle_status_t nacelle_ring_buffer_drop(nacelle_ring_buffer_t *rb, size_t length);

#endif /* NACELLE_RING_BUFFER_H */
