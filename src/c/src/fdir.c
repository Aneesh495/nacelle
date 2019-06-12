#include "nacelle/fdir.h"

void nacelle_fdir_init(nacelle_fdir_engine_t *engine) {
    if (!engine) return;
    engine->channel_count = 0;
    engine->total_anomalies_detected = 0;
    engine->total_mitigations_executed = 0;
    engine->system_in_safe_hold = false;
}

nacelle_channel_severity_t nacelle_fdir_evaluate_severity(uint16_t value, const nacelle_channel_thresholds_t *thresh) {
    if (!thresh) return NACELLE_SEVERITY_NOMINAL;
    if (value < thresh->crit_low || value > thresh->crit_high) {
        return NACELLE_SEVERITY_CRITICAL;
    }
    if (value < thresh->warn_low || value > thresh->warn_high) {
        return NACELLE_SEVERITY_WARNING;
    }
    return NACELLE_SEVERITY_NOMINAL;
}

nacelle_status_t nacelle_fdir_register_channel(nacelle_fdir_engine_t *engine,
                                               uint16_t channel_id,
                                               uint16_t debounce_limit,
                                               const nacelle_channel_thresholds_t *thresh) {
    if (!engine || !thresh) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (engine->channel_count >= NACELLE_MAX_FDIR_CHANNELS) {
        return NACELLE_ERR_QUEUE_FULL;
    }

    size_t idx = engine->channel_count;
    engine->channels[idx].channel_id = channel_id;
    engine->channels[idx].raw_value = 0;
    engine->channels[idx].severity = NACELLE_SEVERITY_NOMINAL;
    engine->channels[idx].anomaly_counter = 0;
    engine->channels[idx].debounce_limit = (debounce_limit > 0) ? debounce_limit : 3;
    engine->channels[idx].escalation_tier = 0;
    engine->channels[idx].thresholds = *thresh;

    engine->channel_count++;
    return NACELLE_OK;
}

nacelle_status_t nacelle_fdir_update_channel(nacelle_fdir_engine_t *engine,
                                             uint16_t channel_id,
                                             uint16_t sample_value,
                                             nacelle_recovery_action_t *out_action) {
    if (!engine || !out_action) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    *out_action = NACELLE_ACTION_NONE;

    nacelle_monitor_channel_t *ch = NULL;
    for (size_t i = 0; i < engine->channel_count; ++i) {
        if (engine->channels[i].channel_id == channel_id) {
            ch = &engine->channels[i];
            break;
        }
    }

    if (!ch) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    ch->raw_value = sample_value;
    nacelle_channel_severity_t sev = nacelle_fdir_evaluate_severity(sample_value, &ch->thresholds);
    ch->severity = sev;

    if (sev == NACELLE_SEVERITY_NOMINAL) {
        if (ch->anomaly_counter > 0) {
            ch->anomaly_counter--;
        }
        return NACELLE_OK;
    }

    /* Anomaly detected: increment debounce filter */
    engine->total_anomalies_detected++;
    if (ch->anomaly_counter < 65535) {
        ch->anomaly_counter++;
    }

    if (ch->anomaly_counter >= ch->debounce_limit) {
        /* Debounce limit tripped: escalate recovery tier */
        if (ch->escalation_tier < 3) {
            ch->escalation_tier++;
        }

        switch (ch->escalation_tier) {
            case 1:
                *out_action = NACELLE_ACTION_COMPONENT_RESET;
                break;
            case 2:
                *out_action = NACELLE_ACTION_SWITCH_SECONDARY;
                break;
            case 3:
            default:
                *out_action = NACELLE_ACTION_SAFE_HOLD;
                engine->system_in_safe_hold = true;
                break;
        }

        ch->anomaly_counter = 0;
        engine->total_mitigations_executed++;
    }

    return NACELLE_OK;
}
