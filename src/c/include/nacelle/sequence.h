#ifndef NACELLE_SEQUENCE_H
#define NACELLE_SEQUENCE_H

#include "nacelle/types.h"

typedef enum {
    NACELLE_SEQ_STATE_OPEN    = 0,
    NACELLE_SEQ_STATE_WAIT    = 1,
    NACELLE_SEQ_STATE_LOCKOUT = 2
} nacelle_seq_state_t;

typedef enum {
    NACELLE_FRAME_SEQUENCED      = 0,
    NACELLE_FRAME_BYPASS         = 1,
    NACELLE_FRAME_CONTROL_UNLOCK = 2,
    NACELLE_FRAME_CONTROL_SET_VR = 3
} nacelle_frame_category_t;

typedef struct {
    nacelle_frame_category_t category;
    uint8_t virtual_chan;
    uint8_t frame_seq_num;
    uint16_t payload_len;
} nacelle_frame_header_t;

typedef struct {
    nacelle_seq_state_t state;
    uint8_t expected_seq;
    uint8_t window_width;
    bool retransmit_req;
    uint32_t accepted_count;
    uint32_t rejected_count;
} nacelle_receiver_status_t;

typedef struct {
    uint8_t virtual_chan;
    bool lockout;
    bool wait_flag;
    bool retransmit;
    uint8_t report_val_vr;
} nacelle_link_report_t;

void nacelle_receiver_init(nacelle_receiver_status_t *status, uint8_t window_width);
nacelle_status_t nacelle_receiver_process_frame(nacelle_receiver_status_t *status, const nacelle_frame_header_t *frame, bool *out_accepted);
void nacelle_generate_link_report(const nacelle_receiver_status_t *status, uint8_t vcid, nacelle_link_report_t *out_report);
uint32_t nacelle_pack_report_word(const nacelle_link_report_t *report);
void nacelle_unpack_report_word(uint32_t word, nacelle_link_report_t *out_report);

#endif /* NACELLE_SEQUENCE_H */
