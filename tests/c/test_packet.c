#include "nacelle/packet.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_packet_header_roundtrip(void) {
    nacelle_packet_header_t hdr = {
        .version = 0,
        .kind = NACELLE_KIND_COMMAND,
        .sec_header = false,
        .channel = 42,
        .grouping = NACELLE_GROUP_STANDALONE,
        .seq_count = 1234,
        .data_len_field = 15
    };

    uint8_t buf[NACELLE_PACKET_HEADER_LEN];
    nacelle_status_t st = nacelle_packet_serialize_header(&hdr, buf, sizeof(buf));
    assert(st == NACELLE_OK);

    nacelle_packet_header_t parsed_hdr;
    st = nacelle_packet_parse_header(buf, sizeof(buf), &parsed_hdr);
    assert(st == NACELLE_OK);

    assert(parsed_hdr.version == hdr.version);
    assert(parsed_hdr.kind == hdr.kind);
    assert(parsed_hdr.sec_header == hdr.sec_header);
    assert(parsed_hdr.channel == hdr.channel);
    assert(parsed_hdr.grouping == hdr.grouping);
    assert(parsed_hdr.seq_count == hdr.seq_count);
    assert(parsed_hdr.data_len_field == hdr.data_len_field);
}

static void test_packet_full_roundtrip(void) {
    nacelle_packet_header_t hdr = {
        .version = 0,
        .kind = NACELLE_KIND_TELEMETRY,
        .sec_header = true,
        .channel = 100,
        .grouping = NACELLE_GROUP_FIRST,
        .seq_count = 55,
        .data_len_field = 0 /* will be overwritten by serializer */
    };

    uint8_t payload[16] = "FlightTelemetry";
    uint8_t wire[64];
    size_t written = 0;

    nacelle_status_t st = nacelle_packet_serialize(&hdr, payload, sizeof(payload), wire, sizeof(wire), &written);
    assert(st == NACELLE_OK);
    assert(written == NACELLE_PACKET_HEADER_LEN + sizeof(payload));

    nacelle_packet_view_t view;
    st = nacelle_packet_parse(wire, written, &view);
    assert(st == NACELLE_OK);
    assert(view.payload_len == sizeof(payload));
    assert(memcmp(view.payload, payload, sizeof(payload)) == 0);
    assert(view.header.channel == 100);
}

static void test_packet_truncated_rejection(void) {
    uint8_t buf[4] = {0x18, 0x2A, 0xC0, 0x01};
    nacelle_packet_header_t hdr;
    nacelle_status_t st = nacelle_packet_parse_header(buf, sizeof(buf), &hdr);
    assert(st == NACELLE_ERR_BUFFER_UNDERFLOW);

    uint8_t wire[8] = {0x18, 0x2A, 0xC0, 0x01, 0x00, 0x10, 0x01, 0x02};
    nacelle_packet_view_t view;
    st = nacelle_packet_parse(wire, sizeof(wire), &view);
    assert(st == NACELLE_ERR_BUFFER_UNDERFLOW);
}

int main(void) {
    printf("[TEST] Running packet parser and serializer unit tests...\n");
    test_packet_header_roundtrip();
    test_packet_full_roundtrip();
    test_packet_truncated_rejection();
    printf("[PASS] All packet unit tests passed.\n");
    return 0;
}
