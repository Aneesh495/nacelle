#include "nacelle/executive.h"
#include <stdio.h>
#include <assert.h>

static int s_count_10hz = 0;
static int s_count_5hz  = 0;
static int s_count_2hz  = 0;

static void task_10hz_fn(void *ctx) {
    (void)ctx;
    s_count_10hz++;
}

static void task_5hz_fn(void *ctx) {
    (void)ctx;
    s_count_5hz++;
}

static void task_2hz_fn(void *ctx) {
    (void)ctx;
    s_count_2hz++;
}

static void test_executive_harmonic_cycles(void) {
    nacelle_executive_t exec;
    nacelle_executive_init(&exec, 42);

    assert(nacelle_executive_register_task(&exec, "t10", task_10hz_fn, NULL, 1, 10) == NACELLE_OK);
    assert(nacelle_executive_register_task(&exec, "t5",  task_5hz_fn,  NULL, 2, 20) == NACELLE_OK);
    assert(nacelle_executive_register_task(&exec, "t2",  task_2hz_fn,  NULL, 5, 30) == NACELLE_OK);

    assert(exec.task_count == 3);
    assert(exec.current_major_cycle == 0);
    assert(exec.current_minor_cycle == 0);

    /* Step through one full major cycle (10 minor cycles) */
    for (int i = 0; i < 10; ++i) {
        assert(nacelle_executive_step_minor(&exec) == NACELLE_OK);
    }

    /* Verify harmonic frequencies */
    assert(s_count_10hz == 10);
    assert(s_count_5hz == 5);
    assert(s_count_2hz == 2);

    /* Major cycle should have wrapped to 1, minor cycle to 0 */
    assert(exec.current_major_cycle == 1);
    assert(exec.current_minor_cycle == 0);
    assert(exec.total_dispatches == 17);
}

int main(void) {
    printf("[TEST] Running cyclic executive scheduler unit tests...\n");
    test_executive_harmonic_cycles();
    printf("[PASS] All cyclic executive unit tests passed.\n");
    return 0;
}
