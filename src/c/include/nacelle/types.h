#ifndef NACELLE_TYPES_H
#define NACELLE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    NACELLE_OK                      = 0,
    NACELLE_ERR_INVALID_PARAM       = 1,
    NACELLE_ERR_BUFFER_OVERFLOW     = 2,
    NACELLE_ERR_BUFFER_UNDERFLOW    = 3,
    NACELLE_ERR_CHECKSUM_MISMATCH   = 4,
    NACELLE_ERR_OUT_OF_SEQUENCE     = 5,
    NACELLE_ERR_QUEUE_FULL          = 6,
    NACELLE_ERR_QUEUE_EMPTY         = 7,
    NACELLE_ERR_LOCKOUT             = 8,
    NACELLE_ERR_TIMEOUT             = 9,
    NACELLE_ERR_ANOMALY_DETECTED    = 10
} nacelle_status_t;

typedef enum {
    NACELLE_PRIORITY_CRITICAL   = 0,
    NACELLE_PRIORITY_HIGH       = 1,
    NACELLE_PRIORITY_NORMAL     = 2,
    NACELLE_PRIORITY_BACKGROUND = 3
} nacelle_priority_t;

typedef struct {
    uint32_t coarse_secs;
    uint16_t fine_ticks;
} nacelle_timestamp_t;

static inline int nacelle_timestamp_compare(nacelle_timestamp_t t1, nacelle_timestamp_t t2) {
    if (t1.coarse_secs < t2.coarse_secs) return -1;
    if (t1.coarse_secs > t2.coarse_secs) return 1;
    if (t1.fine_ticks < t2.fine_ticks) return -1;
    if (t1.fine_ticks > t2.fine_ticks) return 1;
    return 0;
}

static inline uint16_t nacelle_read_u16_be(const uint8_t *b) {
    return (uint16_t)(((uint16_t)b[0] << 8) | (uint16_t)b[1]);
}

static inline void nacelle_write_u16_be(uint8_t *b, uint16_t val) {
    b[0] = (uint8_t)((val >> 8) & 0xFF);
    b[1] = (uint8_t)(val & 0xFF);
}

static inline uint32_t nacelle_read_u32_be(const uint8_t *b) {
    return ((uint32_t)b[0] << 24) |
           ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8)  |
           ((uint32_t)b[3]);
}

static inline void nacelle_write_u32_be(uint8_t *b, uint32_t val) {
    b[0] = (uint8_t)((val >> 24) & 0xFF);
    b[1] = (uint8_t)((val >> 16) & 0xFF);
    b[2] = (uint8_t)((val >> 8)  & 0xFF);
    b[3] = (uint8_t)(val & 0xFF);
}

#endif /* NACELLE_TYPES_H */
