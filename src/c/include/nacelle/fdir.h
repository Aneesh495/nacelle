#ifndef NACELLE_FDIR_H
#define NACELLE_FDIR_H

#include "nacelle/types.h"

#define NACELLE_MAX_FDIR_CHANNELS 16

typedef enum {
    NACELLE_ACTION_NONE            = 0,
    NACELLE_ACTION_COMPONENT_RESET = 1,
    NACELLE_ACTION_SWITCH_SECONDARY= 2,
    NACELLE_ACTION_SAFE_HOLD       = 3
} nacelle_recovery_action_t;

typedef enum {
    NACELLE_SEVERITY_NOMINAL  = 0,
    NACELLE_SEVERITY_WARNING  = 1,
    NACELLE_SEVERITY_CRITICAL = 2
} nacelle_channel_severity_t;

typedef struct {
    uint16_t warn_low;
    uint16_t warn_high;
    uint16_t crit_low;
    uint16_t crit_high;
} nacelle_channel_thresholds_t;

typedef struct {
    uint16_t channel_id;
    uint16_t raw_value;
    nacelle_channel_severity_t severity;
    uint16_t anomaly_counter;
    uint16_t debounce_limit;
    uint8_t escalation_tier;
    nacelle_channel_thresholds_t thresholds;
} nacelle_monitor_channel_t;

typedef struct {
    nacelle_monitor_channel_t channels[NACELLE_MAX_FDIR_CHANNELS];
    size_t channel_count;
    uint32_t total_anomalies_detected;
    uint32_t total_mitigations_executed;
    bool system_in_safe_hold;
} nacelle_fdir_engine_t;

void nacelle_fdir_init(nacelle_fdir_engine_t *engine);
nacelle_status_t nacelle_fdir_register_channel(nacelle_fdir_engine_t *engine,
                                               uint16_t channel_id,
                                               uint16_t debounce_limit,
                                               const nacelle_channel_thresholds_t *thresh);

nacelle_channel_severity_t nacelle_fdir_evaluate_severity(uint16_t value, const nacelle_channel_thresholds_t *thresh);
nacelle_status_t nacelle_fdir_update_channel(nacelle_fdir_engine_t *engine,
                                             uint16_t channel_id,
                                             uint16_t sample_value,
                                             nacelle_recovery_action_t *out_action);

#endif /* NACELLE_FDIR_H */
