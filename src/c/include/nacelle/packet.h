#ifndef NACELLE_PACKET_H
#define NACELLE_PACKET_H

#include "nacelle/types.h"

#define NACELLE_PACKET_HEADER_LEN 6
#define NACELLE_MAX_PACKET_PAYLOAD 65536
#define NACELLE_MAX_PACKET_LEN (NACELLE_PACKET_HEADER_LEN + NACELLE_MAX_PACKET_PAYLOAD)

typedef enum {
    NACELLE_KIND_TELEMETRY = 0,
    NACELLE_KIND_COMMAND   = 1
} nacelle_packet_kind_t;

typedef enum {
    NACELLE_GROUP_CONTINUATION = 0,
    NACELLE_GROUP_FIRST        = 1,
    NACELLE_GROUP_LAST         = 2,
    NACELLE_GROUP_STANDALONE   = 3
} nacelle_sequence_grouping_t;

typedef struct {
    uint8_t version;
    nacelle_packet_kind_t kind;
    bool sec_header;
    uint16_t channel;
    nacelle_sequence_grouping_t grouping;
    uint16_t seq_count;
    uint16_t data_len_field;
} nacelle_packet_header_t;

typedef struct {
    nacelle_packet_header_t header;
    const uint8_t *payload;
    size_t payload_len;
} nacelle_packet_view_t;

nacelle_status_t nacelle_packet_parse_header(const uint8_t *buffer, size_t buffer_len, nacelle_packet_header_t *out_hdr);
nacelle_status_t nacelle_packet_serialize_header(const nacelle_packet_header_t *hdr, uint8_t *out_buf, size_t buf_capacity);
nacelle_status_t nacelle_packet_parse(const uint8_t *buffer, size_t buffer_len, nacelle_packet_view_t *out_pkt);
nacelle_status_t nacelle_packet_serialize(const nacelle_packet_header_t *hdr, const uint8_t *payload, size_t payload_len, uint8_t *out_buf, size_t buf_capacity, size_t *out_written);

#endif /* NACELLE_PACKET_H */
