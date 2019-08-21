#include "nacelle/types.h"
#include "nacelle/checksum.h"
#include "nacelle/packet.h"
#include "nacelle/sequence.h"
#include "nacelle/telemetry.h"
#include "nacelle/queue.h"
#include "nacelle/fdir.h"
#include "nacelle/ring_buffer.h"
#include "nacelle/executive.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_end_to_end_pipeline(void) {
    nacelle_executive_t exec;
    nacelle_executive_init(&exec, 42);

    /* 1. Create a space packet */
    nacelle_packet_header_t pkt_hdr = {
        .version = 0,
        .kind = NACELLE_KIND_COMMAND,
        .sec_header = false,
        .channel = 1,
        .grouping = NACELLE_GROUP_STANDALONE,
        .seq_count = 0,
        .data_len_field = 0
    };
    uint8_t payload[4] = { 0x10, 0x01, 0xAA, 0x55 }; /* Opcode 0x1001 with args */
    uint8_t pkt_wire[64];
    size_t pkt_len = 0;
    assert(nacelle_packet_serialize(&pkt_hdr, payload, sizeof(payload), pkt_wire, sizeof(pkt_wire), &pkt_len) == NACELLE_OK);

    /* 2. Wrap into a telecommand frame with CRC */
    uint8_t tc_frame[128];
    tc_frame[0] = 0x00; /* Category = Sequenced, VC = 0 */
    tc_frame[1] = 0x00; /* Seq num = 0 */
    memcpy(&tc_frame[2], pkt_wire, pkt_len);
    size_t body_len = 2 + pkt_len;
    uint16_t tc_crc = nacelle_crc16_compute(tc_frame, body_len);
    nacelle_write_u16_be(&tc_frame[body_len], tc_crc);

    /* 3. Ingest telecommand frame */
    assert(nacelle_crc16_verify(tc_frame, body_len, tc_crc));
    nacelle_frame_header_t f_hdr = {
        .category = NACELLE_FRAME_SEQUENCED,
        .virtual_chan = 0,
        .frame_seq_num = tc_frame[1],
        .payload_len = (uint16_t)pkt_len
    };
    bool accepted = false;
    assert(nacelle_receiver_process_frame(&exec.receiver, &f_hdr, &accepted) == NACELLE_OK);
    assert(accepted);
    assert(exec.receiver.expected_seq == 1);

    /* 4. Parse space packet from frame */
    nacelle_packet_view_t view;
    assert(nacelle_packet_parse(&tc_frame[2], f_hdr.payload_len, &view) == NACELLE_OK);
    assert(view.header.channel == 1);

    /* 5. Insert into priority queue */
    nacelle_queue_entry_t q_entry = {
        .command_id = 1,
        .dispatch_time = nacelle_get_monotonic_time(),
        .priority = NACELLE_PRIORITY_NORMAL,
        .channel = view.header.channel,
        .opcode = nacelle_read_u16_be(view.payload),
        .arg_len = (uint8_t)(view.payload_len - 2)
    };
    memcpy(q_entry.args, &view.payload[2], q_entry.arg_len);
    assert(nacelle_queue_insert(&exec.cmd_queue, &q_entry) == NACELLE_OK);

    /* 6. Extract from priority queue */
    nacelle_queue_entry_t popped;
    assert(nacelle_queue_extract_min(&exec.cmd_queue, &popped) == NACELLE_OK);
    assert(popped.command_id == 1);
    assert(popped.opcode == 0x1001);

    /* 7. Generate link status report and construct telemetry transfer frame */
    nacelle_link_report_t rep;
    nacelle_generate_link_report(&exec.receiver, 0, &rep);
    assert(rep.report_val_vr == 1);
    assert(!rep.lockout);

    uint8_t tm_frame[NACELLE_TM_FRAME_SIZE];
    uint8_t hk_data[16] = "HK_TELEMETRY_OK";
    assert(nacelle_tm_build_frame(&exec.tm_mux, 0, 0, hk_data, sizeof(hk_data), &rep, tm_frame) == NACELLE_OK);

    /* 8. Verify telemetry frame CRC */
    uint16_t tm_crc = nacelle_read_u16_be(&tm_frame[NACELLE_TM_FRAME_SIZE - 2]);
    assert(nacelle_crc16_verify(tm_frame, NACELLE_TM_FRAME_SIZE - 2, tm_crc));

    /* 9. Store frame in ring buffer */
    assert(nacelle_ring_buffer_write(&exec.telemetry_log, tm_frame, 64) == NACELLE_OK);
    assert(nacelle_ring_buffer_available(&exec.telemetry_log) == 64);
}

int main(void) {
    printf("[TEST] Running end-to-end C integration test suite...\n");
    test_end_to_end_pipeline();
    printf("[PASS] End-to-end integration test passed successfully.\n");
    return 0;
}
