#ifndef NACELLE_EXECUTIVE_H
#define NACELLE_EXECUTIVE_H

#include "nacelle/types.h"
#include "nacelle/queue.h"
#include "nacelle/sequence.h"
#include "nacelle/telemetry.h"
#include "nacelle/fdir.h"
#include "nacelle/ring_buffer.h"

#define NACELLE_MAJOR_CYCLE_MS 1000
#define NACELLE_MINOR_CYCLE_MS 100
#define NACELLE_MINOR_CYCLES_PER_MAJOR 10

typedef void (*nacelle_task_fn_t)(void *context);

typedef struct {
    const char *name;
    nacelle_task_fn_t function;
    void *context;
    uint8_t period_minors;
    uint32_t budget_ms;
    bool enabled;
    uint32_t execution_count;
    uint32_t overrun_count;
} nacelle_task_t;

typedef struct {
    uint32_t current_major_cycle;
    uint8_t current_minor_cycle;
    uint32_t missed_deadlines;
    uint32_t total_dispatches;
    nacelle_task_t tasks[8];
    size_t task_count;
    bool running;

    /* Subsystem handles */
    nacelle_priority_queue_t cmd_queue;
    nacelle_receiver_status_t receiver;
    nacelle_multiplexer_t tm_mux;
    nacelle_fdir_engine_t fdir;
    nacelle_ring_buffer_t telemetry_log;
} nacelle_executive_t;

void nacelle_executive_init(nacelle_executive_t *exec, uint16_t scid);
nacelle_status_t nacelle_executive_register_task(nacelle_executive_t *exec,
                                                 const char *name,
                                                 nacelle_task_fn_t fn,
                                                 void *context,
                                                 uint8_t period_minors,
                                                 uint32_t budget_ms);

nacelle_status_t nacelle_executive_step_minor(nacelle_executive_t *exec);
nacelle_timestamp_t nacelle_get_monotonic_time(void);

#endif /* NACELLE_EXECUTIVE_H */
