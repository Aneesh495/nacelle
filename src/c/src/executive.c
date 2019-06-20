#include "nacelle/executive.h"
#include <time.h>
#include <string.h>

nacelle_timestamp_t nacelle_get_monotonic_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    nacelle_timestamp_t t;
    t.coarse_secs = (uint32_t)ts.tv_sec;
    /* Convert nanoseconds (0..999,999,999) to 16-bit fine ticks (0..65535) */
    t.fine_ticks = (uint16_t)(((uint64_t)ts.tv_nsec * 65536ULL) / 1000000000ULL);
    return t;
}

void nacelle_executive_init(nacelle_executive_t *exec, uint16_t scid) {
    if (!exec) return;
    exec->current_major_cycle = 0;
    exec->current_minor_cycle = 0;
    exec->missed_deadlines = 0;
    exec->total_dispatches = 0;
    exec->task_count = 0;
    exec->running = true;

    nacelle_queue_init(&exec->cmd_queue);
    nacelle_receiver_init(&exec->receiver, 64);
    nacelle_multiplexer_init(&exec->tm_mux, scid);
    nacelle_fdir_init(&exec->fdir);
    nacelle_ring_buffer_init(&exec->telemetry_log);
}

nacelle_status_t nacelle_executive_register_task(nacelle_executive_t *exec,
                                                 const char *name,
                                                 nacelle_task_fn_t fn,
                                                 void *context,
                                                 uint8_t period_minors,
                                                 uint32_t budget_ms) {
    if (!exec || !fn) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (exec->task_count >= 8) {
        return NACELLE_ERR_QUEUE_FULL;
    }

    size_t idx = exec->task_count;
    exec->tasks[idx].name = name ? name : "unnamed_task";
    exec->tasks[idx].function = fn;
    exec->tasks[idx].context = context;
    exec->tasks[idx].period_minors = (period_minors > 0) ? period_minors : 1;
    exec->tasks[idx].budget_ms = budget_ms;
    exec->tasks[idx].enabled = true;
    exec->tasks[idx].execution_count = 0;
    exec->tasks[idx].overrun_count = 0;

    exec->task_count++;
    return NACELLE_OK;
}

nacelle_status_t nacelle_executive_step_minor(nacelle_executive_t *exec) {
    if (!exec) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    uint8_t minor = exec->current_minor_cycle;

    /* Execute all tasks scheduled for this minor cycle */
    for (size_t i = 0; i < exec->task_count; ++i) {
        nacelle_task_t *task = &exec->tasks[i];
        if (task->enabled && (minor % task->period_minors == 0)) {
            task->function(task->context);
            task->execution_count++;
            exec->total_dispatches++;
        }
    }

    /* Advance minor cycle index */
    uint8_t next_minor = (uint8_t)((minor + 1) % NACELLE_MINOR_CYCLES_PER_MAJOR);
    if (next_minor == 0) {
        exec->current_major_cycle++;
    }
    exec->current_minor_cycle = next_minor;

    return NACELLE_OK;
}
