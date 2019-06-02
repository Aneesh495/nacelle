#include "nacelle/sequence.h"

static inline bool in_positive_window(uint8_t ns, uint8_t vr, uint8_t w) {
    uint8_t diff = (uint8_t)(ns - vr);
    return (diff < w);
}

void nacelle_receiver_init(nacelle_receiver_status_t *status, uint8_t window_width) {
    if (!status) return;
    status->state = NACELLE_SEQ_STATE_OPEN;
    status->expected_seq = 0;
    status->window_width = (window_width > 0) ? window_width : 64;
    status->retransmit_req = false;
    status->accepted_count = 0;
    status->rejected_count = 0;
}

nacelle_status_t nacelle_receiver_process_frame(nacelle_receiver_status_t *status, const nacelle_frame_header_t *frame, bool *out_accepted) {
    if (!status || !frame || !out_accepted) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    *out_accepted = false;

    switch (status->state) {
        case NACELLE_SEQ_STATE_LOCKOUT:
            if (frame->category == NACELLE_FRAME_CONTROL_UNLOCK) {
                status->state = NACELLE_SEQ_STATE_OPEN;
                status->retransmit_req = false;
                status->accepted_count++;
                *out_accepted = true;
                return NACELLE_OK;
            }
            status->rejected_count++;
            return NACELLE_ERR_LOCKOUT;

        case NACELLE_SEQ_STATE_WAIT:
            if (frame->category == NACELLE_FRAME_CONTROL_UNLOCK) {
                status->state = NACELLE_SEQ_STATE_OPEN;
                status->retransmit_req = false;
                status->accepted_count++;
                *out_accepted = true;
                return NACELLE_OK;
            }
            if (frame->category == NACELLE_FRAME_BYPASS) {
                status->accepted_count++;
                *out_accepted = true;
                return NACELLE_OK;
            }
            if (frame->category == NACELLE_FRAME_SEQUENCED) {
                if (frame->frame_seq_num == status->expected_seq) {
                    status->state = NACELLE_SEQ_STATE_OPEN;
                    status->expected_seq = (uint8_t)(status->expected_seq + 1);
                    status->retransmit_req = false;
                    status->accepted_count++;
                    *out_accepted = true;
                    return NACELLE_OK;
                }
                status->rejected_count++;
                return NACELLE_ERR_OUT_OF_SEQUENCE;
            }
            status->rejected_count++;
            return NACELLE_ERR_INVALID_PARAM;

        case NACELLE_SEQ_STATE_OPEN:
            if (frame->category == NACELLE_FRAME_BYPASS) {
                status->accepted_count++;
                *out_accepted = true;
                return NACELLE_OK;
            }
            if (frame->category == NACELLE_FRAME_CONTROL_UNLOCK) {
                status->retransmit_req = false;
                status->accepted_count++;
                *out_accepted = true;
                return NACELLE_OK;
            }
            if (frame->category == NACELLE_FRAME_CONTROL_SET_VR) {
                status->expected_seq = frame->frame_seq_num;
                status->retransmit_req = false;
                status->accepted_count++;
                *out_accepted = true;
                return NACELLE_OK;
            }
            if (frame->category == NACELLE_FRAME_SEQUENCED) {
                if (frame->frame_seq_num == status->expected_seq) {
                    status->expected_seq = (uint8_t)(status->expected_seq + 1);
                    status->accepted_count++;
                    *out_accepted = true;
                    return NACELLE_OK;
                }
                if (in_positive_window(frame->frame_seq_num, status->expected_seq, status->window_width)) {
                    status->state = NACELLE_SEQ_STATE_WAIT;
                    status->retransmit_req = true;
                    status->rejected_count++;
                    return NACELLE_ERR_OUT_OF_SEQUENCE;
                }
                status->state = NACELLE_SEQ_STATE_LOCKOUT;
                status->rejected_count++;
                return NACELLE_ERR_LOCKOUT;
            }
            status->rejected_count++;
            return NACELLE_ERR_INVALID_PARAM;
    }

    return NACELLE_ERR_INVALID_PARAM;
}

void nacelle_generate_link_report(const nacelle_receiver_status_t *status, uint8_t vcid, nacelle_link_report_t *out_report) {
    if (!status || !out_report) return;
    out_report->virtual_chan = (uint8_t)(vcid & 0x3F);
    out_report->lockout = (status->state == NACELLE_SEQ_STATE_LOCKOUT);
    out_report->wait_flag = (status->state == NACELLE_SEQ_STATE_WAIT);
    out_report->retransmit = status->retransmit_req;
    out_report->report_val_vr = status->expected_seq;
}

uint32_t nacelle_pack_report_word(const nacelle_link_report_t *report) {
    if (!report) return 0;
    uint32_t word = ((uint32_t)(report->virtual_chan & 0x3F) << 24) |
                    ((report->lockout ? 1U : 0U) << 23) |
                    ((report->wait_flag ? 1U : 0U) << 22) |
                    ((report->retransmit ? 1U : 0U) << 21) |
                    ((uint32_t)report->report_val_vr);
    return word;
}

void nacelle_unpack_report_word(uint32_t word, nacelle_link_report_t *out_report) {
    if (!out_report) return;
    out_report->virtual_chan = (uint8_t)((word >> 24) & 0x3F);
    out_report->lockout = ((word & (1U << 23)) != 0);
    out_report->wait_flag = ((word & (1U << 22)) != 0);
    out_report->retransmit = ((word & (1U << 21)) != 0);
    out_report->report_val_vr = (uint8_t)(word & 0xFF);
}
