#include "nacelle/telemetry.h"
#include "nacelle/checksum.h"
#include <string.h>

void nacelle_multiplexer_init(nacelle_multiplexer_t *mux, uint16_t scid) {
    if (!mux) return;
    mux->spacecraft_id = scid;
    mux->next_master_seq = 0;
    for (int i = 0; i < 8; ++i) {
        mux->vc_states[i].virtual_channel = (uint8_t)i;
        mux->vc_states[i].next_virt_seq = 0;
    }
}

size_t nacelle_generate_idle_pattern(uint8_t *buffer, size_t length) {
    if (!buffer) return 0;
    for (size_t i = 0; i < length; ++i) {
        buffer[i] = (uint8_t)((i % 2 == 0) ? 0x55 : 0xAA);
    }
    return length;
}

nacelle_status_t nacelle_tm_build_frame(nacelle_multiplexer_t *mux,
                                        uint8_t vcid,
                                        uint16_t first_header_ptr,
                                        const uint8_t *data,
                                        size_t data_len,
                                        const nacelle_link_report_t *report,
                                        uint8_t *out_frame) {
    if (!mux || !out_frame) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (data_len > NACELLE_TM_DATA_FIELD_SIZE) {
        return NACELLE_ERR_BUFFER_OVERFLOW;
    }

    uint8_t vc_index = (uint8_t)(vcid % 8);
    uint8_t m_seq = mux->next_master_seq;
    uint8_t v_seq = mux->vc_states[vc_index].next_virt_seq;

    mux->next_master_seq = (uint8_t)(mux->next_master_seq + 1);
    mux->vc_states[vc_index].next_virt_seq = (uint8_t)(mux->vc_states[vc_index].next_virt_seq + 1);

    /* Construct 6-byte telemetry transfer frame header */
    uint16_t w0 = (uint16_t)(((mux->spacecraft_id & 0x03FFU) << 4) | ((vcid & 0x07U) << 1) | (report ? 1U : 0U));
    uint8_t b2 = m_seq;
    uint8_t b3 = v_seq;
    uint16_t w4 = (uint16_t)(first_header_ptr & 0x07FFU);

    nacelle_write_u16_be(&out_frame[0], w0);
    out_frame[2] = b2;
    out_frame[3] = b3;
    nacelle_write_u16_be(&out_frame[4], w4);

    /* Copy payload into data field */
    uint8_t *data_dest = &out_frame[NACELLE_TM_HEADER_SIZE];
    if (data && data_len > 0) {
        memcpy(data_dest, data, data_len);
    }

    /* Pad remaining bytes in data field with deterministic idle pattern */
    if (data_len < NACELLE_TM_DATA_FIELD_SIZE) {
        size_t pad_len = NACELLE_TM_DATA_FIELD_SIZE - data_len;
        nacelle_generate_idle_pattern(&data_dest[data_len], pad_len);
    }

    /* Append Operational Control Field (4-byte link report word) */
    size_t report_offset = NACELLE_TM_HEADER_SIZE + NACELLE_TM_DATA_FIELD_SIZE;
    if (report) {
        uint32_t rep_word = nacelle_pack_report_word(report);
        nacelle_write_u32_be(&out_frame[report_offset], rep_word);
    } else {
        memset(&out_frame[report_offset], 0, NACELLE_TM_REPORT_SIZE);
    }

    /* Calculate and append CRC16 over the frame up to the error control field */
    size_t crc_offset = report_offset + NACELLE_TM_REPORT_SIZE;
    uint16_t crc = nacelle_crc16_compute(out_frame, crc_offset);
    nacelle_write_u16_be(&out_frame[crc_offset], crc);

    return NACELLE_OK;
}
