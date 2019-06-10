#ifndef NACELLE_QUEUE_H
#define NACELLE_QUEUE_H

#include "nacelle/types.h"

#define NACELLE_MAX_QUEUE_CAPACITY 256
#define NACELLE_MAX_CMD_ARG_LEN    32

typedef struct {
    uint32_t command_id;
    nacelle_timestamp_t dispatch_time;
    nacelle_priority_t priority;
    uint16_t channel;
    uint16_t opcode;
    uint8_t arg_len;
    uint8_t args[NACELLE_MAX_CMD_ARG_LEN];
} nacelle_queue_entry_t;

typedef struct {
    nacelle_queue_entry_t entries[NACELLE_MAX_QUEUE_CAPACITY];
    size_t count;
} nacelle_priority_queue_t;

void nacelle_queue_init(nacelle_priority_queue_t *q);
bool nacelle_queue_is_empty(const nacelle_priority_queue_t *q);
bool nacelle_queue_is_full(const nacelle_priority_queue_t *q);
size_t nacelle_queue_size(const nacelle_priority_queue_t *q);

nacelle_status_t nacelle_queue_insert(nacelle_priority_queue_t *q, const nacelle_queue_entry_t *entry);
nacelle_status_t nacelle_queue_peek(const nacelle_priority_queue_t *q, nacelle_queue_entry_t *out_entry);
nacelle_status_t nacelle_queue_extract_min(nacelle_priority_queue_t *q, nacelle_queue_entry_t *out_entry);

#endif /* NACELLE_QUEUE_H */
