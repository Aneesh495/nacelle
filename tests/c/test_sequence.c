#include "nacelle/sequence.h"
#include <stdio.h>
#include <assert.h>

static void test_sequence_nominal_progression(void) {
    nacelle_receiver_status_t rx;
    nacelle_receiver_init(&rx, 32);

    for (int i = 0; i < 260; ++i) {
        nacelle_frame_header_t f = {
            .category = NACELLE_FRAME_SEQUENCED,
            .virtual_chan = 0,
            .frame_seq_num = (uint8_t)(i % 256),
            .payload_len = 10
        };

        bool accepted = false;
        nacelle_status_t st = nacelle_receiver_process_frame(&rx, &f, &accepted);
        assert(st == NACELLE_OK);
        assert(accepted);
        assert(rx.expected_seq == (uint8_t)((i + 1) % 256));
    }
    assert(rx.accepted_count == 260);
    assert(rx.rejected_count == 0);
}

static void test_sequence_out_of_order_wait_state(void) {
    nacelle_receiver_status_t rx;
    nacelle_receiver_init(&rx, 32);

    /* Frame 0 accepted */
    nacelle_frame_header_t f0 = { NACELLE_FRAME_SEQUENCED, 0, 0, 10 };
    bool accepted = false;
    nacelle_receiver_process_frame(&rx, &f0, &accepted);
    assert(accepted);

    /* Frame 5 arrives (skipping 1..4), should enter WAIT state */
    nacelle_frame_header_t f5 = { NACELLE_FRAME_SEQUENCED, 0, 5, 10 };
    nacelle_status_t st = nacelle_receiver_process_frame(&rx, &f5, &accepted);
    assert(st == NACELLE_ERR_OUT_OF_SEQUENCE);
    assert(!accepted);
    assert(rx.state == NACELLE_SEQ_STATE_WAIT);
    assert(rx.retransmit_req == true);

    /* Sending expected frame 1 should recover state to OPEN */
    nacelle_frame_header_t f1 = { NACELLE_FRAME_SEQUENCED, 0, 1, 10 };
    st = nacelle_receiver_process_frame(&rx, &f1, &accepted);
    assert(st == NACELLE_OK);
    assert(accepted);
    assert(rx.state == NACELLE_SEQ_STATE_OPEN);
    assert(rx.retransmit_req == false);
    assert(rx.expected_seq == 2);
}

static void test_sequence_lockout_and_unlock(void) {
    nacelle_receiver_status_t rx;
    nacelle_receiver_init(&rx, 16);

    /* Send frame way outside window (e.g. 100 when expected is 0 and window is 16) */
    nacelle_frame_header_t f100 = { NACELLE_FRAME_SEQUENCED, 0, 100, 10 };
    bool accepted = false;
    nacelle_status_t st = nacelle_receiver_process_frame(&rx, &f100, &accepted);
    assert(st == NACELLE_ERR_LOCKOUT);
    assert(!accepted);
    assert(rx.state == NACELLE_SEQ_STATE_LOCKOUT);

    /* Subsequent frames should be rejected */
    nacelle_frame_header_t f0 = { NACELLE_FRAME_SEQUENCED, 0, 0, 10 };
    st = nacelle_receiver_process_frame(&rx, &f0, &accepted);
    assert(st == NACELLE_ERR_LOCKOUT);
    assert(!accepted);

    /* Unlock control frame clears lockout */
    nacelle_frame_header_t funlock = { NACELLE_FRAME_CONTROL_UNLOCK, 0, 0, 0 };
    st = nacelle_receiver_process_frame(&rx, &funlock, &accepted);
    assert(st == NACELLE_OK);
    assert(accepted);
    assert(rx.state == NACELLE_SEQ_STATE_OPEN);
}

static void test_link_report_packing(void) {
    nacelle_link_report_t rep = {
        .virtual_chan = 7,
        .lockout = true,
        .wait_flag = false,
        .retransmit = true,
        .report_val_vr = 42
    };

    uint32_t packed = nacelle_pack_report_word(&rep);
    nacelle_link_report_t unpacked;
    nacelle_unpack_report_word(packed, &unpacked);

    assert(unpacked.virtual_chan == rep.virtual_chan);
    assert(unpacked.lockout == rep.lockout);
    assert(unpacked.wait_flag == rep.wait_flag);
    assert(unpacked.retransmit == rep.retransmit);
    assert(unpacked.report_val_vr == rep.report_val_vr);
}

int main(void) {
    printf("[TEST] Running sequence validation state machine unit tests...\n");
    test_sequence_nominal_progression();
    test_sequence_out_of_order_wait_state();
    test_sequence_lockout_and_unlock();
    test_link_report_packing();
    printf("[PASS] All sequence validation unit tests passed.\n");
    return 0;
}
