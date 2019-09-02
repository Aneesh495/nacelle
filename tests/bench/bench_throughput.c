#include "nacelle/types.h"
#include "nacelle/checksum.h"
#include "nacelle/packet.h"
#include "nacelle/queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void bench_crc16(void) {
    const size_t buf_size = 1024 * 1024; /* 1 MB */
    uint8_t *buf = (uint8_t *)malloc(buf_size);
    if (!buf) return;
    memset(buf, 0x5A, buf_size);

    const int iterations = 100;
    double t0 = get_time_sec();
    uint16_t crc = 0;
    for (int i = 0; i < iterations; ++i) {
        crc ^= nacelle_crc16_compute(buf, buf_size);
    }
    double t1 = get_time_sec();
    double elapsed = t1 - t0;
    double total_mb = (double)(buf_size * iterations) / (1024.0 * 1024.0);
    double throughput = total_mb / elapsed;

    printf("[BENCH] CRC16 Throughput: %.2f MB/s (checksum: 0x%04x)\n", throughput, crc);
    free(buf);
}

static void bench_packet_roundtrip(void) {
    nacelle_packet_header_t hdr = {
        .version = 0,
        .kind = NACELLE_KIND_COMMAND,
        .sec_header = false,
        .channel = 42,
        .grouping = NACELLE_GROUP_STANDALONE,
        .seq_count = 100,
        .data_len_field = 0
    };
    uint8_t payload[32];
    memset(payload, 0xA5, sizeof(payload));

    uint8_t wire[64];
    size_t written = 0;

    const int iterations = 1000000; /* 1 Million packets */
    double t0 = get_time_sec();

    for (int i = 0; i < iterations; ++i) {
        hdr.seq_count = (uint16_t)(i & 0x3FFF);
        nacelle_packet_serialize(&hdr, payload, sizeof(payload), wire, sizeof(wire), &written);
        nacelle_packet_view_t view;
        nacelle_packet_parse(wire, written, &view);
    }

    double t1 = get_time_sec();
    double elapsed = t1 - t0;
    double pps = (double)iterations / elapsed;
    double ns_per_pkt = (elapsed / (double)iterations) * 1e9;

    printf("[BENCH] Packet Serialize+Parse: %.2f Mpps (%.1f ns/packet)\n", pps / 1e6, ns_per_pkt);
}

static void bench_queue_ops(void) {
    nacelle_priority_queue_t q;
    nacelle_queue_init(&q);

    const int iterations = 500000;
    double t0 = get_time_sec();

    for (int i = 0; i < iterations; ++i) {
        nacelle_queue_entry_t entry = {
            .command_id = (uint32_t)i,
            .dispatch_time = { (uint32_t)(i % 1000), (uint16_t)(i % 65536) },
            .priority = (nacelle_priority_t)(i % 4),
            .channel = 1,
            .opcode = 0,
            .arg_len = 0,
            .args = {0}
        };
        nacelle_queue_insert(&q, &entry);
        if (q.count >= 200) {
            nacelle_queue_entry_t out;
            while (q.count > 50) {
                nacelle_queue_extract_min(&q, &out);
            }
        }
    }

    double t1 = get_time_sec();
    double elapsed = t1 - t0;
    double ops = (double)iterations / elapsed;

    printf("[BENCH] Priority Heap Ops: %.2f Mops/s\n", ops / 1e6);
}

int main(void) {
    printf("==========================================================\n");
    printf(" Nacelle Performance Benchmark Suite\n");
    printf("==========================================================\n");
    bench_crc16();
    bench_packet_roundtrip();
    bench_queue_ops();
    printf("==========================================================\n");
    return 0;
}
