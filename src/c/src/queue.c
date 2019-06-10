#include "nacelle/queue.h"

static bool entry_has_higher_priority(const nacelle_queue_entry_t *e1, const nacelle_queue_entry_t *e2) {
    int t_cmp = nacelle_timestamp_compare(e1->dispatch_time, e2->dispatch_time);
    if (t_cmp < 0) return true;
    if (t_cmp > 0) return false;

    if (e1->priority < e2->priority) return true;
    if (e1->priority > e2->priority) return false;

    return (e1->command_id < e2->command_id);
}

void nacelle_queue_init(nacelle_priority_queue_t *q) {
    if (!q) return;
    q->count = 0;
}

bool nacelle_queue_is_empty(const nacelle_priority_queue_t *q) {
    return (!q || q->count == 0);
}

bool nacelle_queue_is_full(const nacelle_priority_queue_t *q) {
    return (q && q->count >= NACELLE_MAX_QUEUE_CAPACITY);
}

size_t nacelle_queue_size(const nacelle_priority_queue_t *q) {
    return q ? q->count : 0;
}

static void sift_up(nacelle_priority_queue_t *q, size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (entry_has_higher_priority(&q->entries[index], &q->entries[parent])) {
            nacelle_queue_entry_t temp = q->entries[index];
            q->entries[index] = q->entries[parent];
            q->entries[parent] = temp;
            index = parent;
        } else {
            break;
        }
    }
}

static void sift_down(nacelle_priority_queue_t *q, size_t index) {
    size_t count = q->count;
    while (index * 2 + 1 < count) {
        size_t left = index * 2 + 1;
        size_t right = left + 1;
        size_t best = index;

        if (entry_has_higher_priority(&q->entries[left], &q->entries[best])) {
            best = left;
        }
        if (right < count && entry_has_higher_priority(&q->entries[right], &q->entries[best])) {
            best = right;
        }

        if (best != index) {
            nacelle_queue_entry_t temp = q->entries[index];
            q->entries[index] = q->entries[best];
            q->entries[best] = temp;
            index = best;
        } else {
            break;
        }
    }
}

nacelle_status_t nacelle_queue_insert(nacelle_priority_queue_t *q, const nacelle_queue_entry_t *entry) {
    if (!q || !entry) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (q->count >= NACELLE_MAX_QUEUE_CAPACITY) {
        return NACELLE_ERR_QUEUE_FULL;
    }

    size_t index = q->count;
    q->entries[index] = *entry;
    q->count++;
    sift_up(q, index);

    return NACELLE_OK;
}

nacelle_status_t nacelle_queue_peek(const nacelle_priority_queue_t *q, nacelle_queue_entry_t *out_entry) {
    if (!q || !out_entry) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (q->count == 0) {
        return NACELLE_ERR_QUEUE_EMPTY;
    }

    *out_entry = q->entries[0];
    return NACELLE_OK;
}

nacelle_status_t nacelle_queue_extract_min(nacelle_priority_queue_t *q, nacelle_queue_entry_t *out_entry) {
    if (!q || !out_entry) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (q->count == 0) {
        return NACELLE_ERR_QUEUE_EMPTY;
    }

    *out_entry = q->entries[0];
    q->count--;
    if (q->count > 0) {
        q->entries[0] = q->entries[q->count];
        sift_down(q, 0);
    }

    return NACELLE_OK;
}
