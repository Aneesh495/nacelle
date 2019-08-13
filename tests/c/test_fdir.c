#include "nacelle/fdir.h"
#include <stdio.h>
#include <assert.h>

static void test_fdir_nominal_and_debounce_suppression(void) {
    nacelle_fdir_engine_t fdir;
    nacelle_fdir_init(&fdir);

    nacelle_channel_thresholds_t thresh = {
        .warn_low = 3000, .warn_high = 3500,
        .crit_low = 2800, .crit_high = 3700
    };
    assert(nacelle_fdir_register_channel(&fdir, 1, 3, &thresh) == NACELLE_OK);

    nacelle_recovery_action_t action = NACELLE_ACTION_NONE;

    /* Nominal readings: no action */
    assert(nacelle_fdir_update_channel(&fdir, 1, 3300, &action) == NACELLE_OK);
    assert(action == NACELLE_ACTION_NONE);

    /* Transient anomaly 1: counter = 1 (< debounce 3), no action */
    assert(nacelle_fdir_update_channel(&fdir, 1, 2500, &action) == NACELLE_OK);
    assert(action == NACELLE_ACTION_NONE);

    /* Return to nominal: counter decrements to 0, no action */
    assert(nacelle_fdir_update_channel(&fdir, 1, 3300, &action) == NACELLE_OK);
    assert(action == NACELLE_ACTION_NONE);
}

static void test_fdir_hierarchical_escalation(void) {
    nacelle_fdir_engine_t fdir;
    nacelle_fdir_init(&fdir);

    nacelle_channel_thresholds_t thresh = {
        .warn_low = 3000, .warn_high = 3500,
        .crit_low = 2800, .crit_high = 3700
    };
    assert(nacelle_fdir_register_channel(&fdir, 1, 3, &thresh) == NACELLE_OK);

    nacelle_recovery_action_t action = NACELLE_ACTION_NONE;

    /* Trip Tier 1: 3 persistent anomalies */
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_NONE);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_NONE);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_COMPONENT_RESET);

    /* Trip Tier 2: 3 more persistent anomalies */
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_NONE);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_NONE);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_SWITCH_SECONDARY);

    /* Trip Tier 3: 3 more persistent anomalies -> SAFE HOLD */
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_NONE);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_NONE);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_SAFE_HOLD);
    assert(fdir.system_in_safe_hold);

    /* Further anomalies remain absorbed in safe hold */
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    nacelle_fdir_update_channel(&fdir, 1, 2700, &action);
    assert(action == NACELLE_ACTION_SAFE_HOLD);
    assert(fdir.system_in_safe_hold);
}

int main(void) {
    printf("[TEST] Running FDIR fault mitigation unit tests...\n");
    test_fdir_nominal_and_debounce_suppression();
    test_fdir_hierarchical_escalation();
    printf("[PASS] All FDIR unit tests passed.\n");
    return 0;
}
