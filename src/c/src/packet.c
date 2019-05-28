#include "nacelle/packet.h"
#include <string.h>

nacelle_status_t nacelle_packet_parse_header(const uint8_t *buffer, size_t buffer_len, nacelle_packet_header_t *out_hdr) {
    if (!buffer || !out_hdr) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (buffer_len < NACELLE_PACKET_HEADER_LEN) {
        return NACELLE_ERR_BUFFER_UNDERFLOW;
    }

    uint16_t w0 = nacelle_read_u16_be(&buffer[0]);
    uint16_t w1 = nacelle_read_u16_be(&buffer[2]);
    uint16_t w2 = nacelle_read_u16_be(&buffer[4]);

    out_hdr->version = (uint8_t)((w0 >> 13) & 0x07);
    out_hdr->kind = ((w0 & 0x1000U) != 0) ? NACELLE_KIND_COMMAND : NACELLE_KIND_TELEMETRY;
    out_hdr->sec_header = ((w0 & 0x0800U) != 0);
    out_hdr->channel = (uint16_t)(w0 & 0x07FFU);

    out_hdr->grouping = (nacelle_sequence_grouping_t)((w1 >> 14) & 0x03);
    out_hdr->seq_count = (uint16_t)(w1 & 0x3FFFU);
    out_hdr->data_len_field = w2;

    return NACELLE_OK;
}

nacelle_status_t nacelle_packet_serialize_header(const nacelle_packet_header_t *hdr, uint8_t *out_buf, size_t buf_capacity) {
    if (!hdr || !out_buf) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (buf_capacity < NACELLE_PACKET_HEADER_LEN) {
        return NACELLE_ERR_BUFFER_OVERFLOW;
    }

    uint16_t w0 = (uint16_t)(((uint16_t)(hdr->version & 0x07) << 13) |
                            ((hdr->kind == NACELLE_KIND_COMMAND ? 1U : 0U) << 12) |
                            ((hdr->sec_header ? 1U : 0U) << 11) |
                            (hdr->channel & 0x07FFU));

    uint16_t w1 = (uint16_t)(((uint16_t)(hdr->grouping & 0x03) << 14) |
                            (hdr->seq_count & 0x3FFFU));

    uint16_t w2 = hdr->data_len_field;

    nacelle_write_u16_be(&out_buf[0], w0);
    nacelle_write_u16_be(&out_buf[2], w1);
    nacelle_write_u16_be(&out_buf[4], w2);

    return NACELLE_OK;
}

nacelle_status_t nacelle_packet_parse(const uint8_t *buffer, size_t buffer_len, nacelle_packet_view_t *out_pkt) {
    if (!buffer || !out_pkt) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    nacelle_status_t status = nacelle_packet_parse_header(buffer, buffer_len, &out_pkt->header);
    if (status != NACELLE_OK) {
        return status;
    }

    size_t expected_payload_len = (size_t)out_pkt->header.data_len_field + 1;
    size_t total_required = NACELLE_PACKET_HEADER_LEN + expected_payload_len;

    if (buffer_len < total_required) {
        return NACELLE_ERR_BUFFER_UNDERFLOW;
    }

    out_pkt->payload = &buffer[NACELLE_PACKET_HEADER_LEN];
    out_pkt->payload_len = expected_payload_len;

    return NACELLE_OK;
}

nacelle_status_t nacelle_packet_serialize(const nacelle_packet_header_t *hdr, const uint8_t *payload, size_t payload_len, uint8_t *out_buf, size_t buf_capacity, size_t *out_written) {
    if (!hdr || !out_buf) {
        return NACELLE_ERR_INVALID_PARAM;
    }
    if (payload_len == 0 || payload_len > NACELLE_MAX_PACKET_PAYLOAD) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    size_t total_len = NACELLE_PACKET_HEADER_LEN + payload_len;
    if (buf_capacity < total_len) {
        return NACELLE_ERR_BUFFER_OVERFLOW;
    }

    nacelle_packet_header_t final_hdr = *hdr;
    final_hdr.data_len_field = (uint16_t)(payload_len - 1);

    nacelle_status_t status = nacelle_packet_serialize_header(&final_hdr, out_buf, buf_capacity);
    if (status != NACELLE_OK) {
        return status;
    }

    if (payload) {
        memcpy(&out_buf[NACELLE_PACKET_HEADER_LEN], payload, payload_len);
    }

    if (out_written) {
        *out_written = total_len;
    }

    return NACELLE_OK;
}
