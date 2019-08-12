#include "nacelle/queue.h"
#include <stdio.h>
#include <assert.h>

static void test_queue_ordering(void) {
    nacelle_priority_queue_t q;
    nacelle_queue_init(&q);
    assert(nacelle_queue_is_empty(&q));

    nacelle_queue_entry_t e1 = { .command_id = 1, .dispatch_time = { 100, 0 }, .priority = NACELLE_PRIORITY_NORMAL };
    nacelle_queue_entry_t e2 = { .command_id = 2, .dispatch_time = { 50,  0 }, .priority = NACELLE_PRIORITY_NORMAL };
    nacelle_queue_entry_t e3 = { .command_id = 3, .dispatch_time = { 150, 0 }, .priority = NACELLE_PRIORITY_NORMAL };
    nacelle_queue_entry_t e4 = { .command_id = 4, .dispatch_time = { 50,  0 }, .priority = NACELLE_PRIORITY_CRITICAL };

    assert(nacelle_queue_insert(&q, &e1) == NACELLE_OK);
    assert(nacelle_queue_insert(&q, &e2) == NACELLE_OK);
    assert(nacelle_queue_insert(&q, &e3) == NACELLE_OK);
    assert(nacelle_queue_insert(&q, &e4) == NACELLE_OK);
    assert(nacelle_queue_size(&q) == 4);

    nacelle_queue_entry_t out;
    /* First should be e4: timestamp 50 with CRITICAL priority */
    assert(nacelle_queue_extract_min(&q, &out) == NACELLE_OK);
    assert(out.command_id == 4);

    /* Second should be e2: timestamp 50 with NORMAL priority */
    assert(nacelle_queue_extract_min(&q, &out) == NACELLE_OK);
    assert(out.command_id == 2);

    /* Third should be e1: timestamp 100 */
    assert(nacelle_queue_extract_min(&q, &out) == NACELLE_OK);
    assert(out.command_id == 1);

    /* Fourth should be e3: timestamp 150 */
    assert(nacelle_queue_extract_min(&q, &out) == NACELLE_OK);
    assert(out.command_id == 3);

    assert(nacelle_queue_is_empty(&q));
}

static void test_queue_capacity_limit(void) {
    nacelle_priority_queue_t q;
    nacelle_queue_init(&q);

    for (size_t i = 0; i < NACELLE_MAX_QUEUE_CAPACITY; ++i) {
        nacelle_queue_entry_t e = { .command_id = (uint32_t)i, .dispatch_time = { (uint32_t)i, 0 } };
        assert(nacelle_queue_insert(&q, &e) == NACELLE_OK);
    }
    assert(nacelle_queue_is_full(&q));

    /* 257th insertion must fail with QUEUE_FULL */
    nacelle_queue_entry_t overflow_entry = { .command_id = 999 };
    assert(nacelle_queue_insert(&q, &overflow_entry) == NACELLE_ERR_QUEUE_FULL);
}

int main(void) {
    printf("[TEST] Running priority queue unit tests...\n");
    test_queue_ordering();
    test_queue_capacity_limit();
    printf("[PASS] All priority queue unit tests passed.\n");
    return 0;
}
