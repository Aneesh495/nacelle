#include "nacelle/types.h"
#include "nacelle/checksum.h"
#include "nacelle/packet.h"
#include "nacelle/sequence.h"
#include "nacelle/telemetry.h"
#include "nacelle/queue.h"
#include "nacelle/fdir.h"
#include "nacelle/ring_buffer.h"
#include "nacelle/executive.h"
#include "nacelle/comms.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static volatile bool s_keep_running = true;

static void handle_sigint(int sig) {
    (void)sig;
    s_keep_running = false;
}

typedef struct {
    nacelle_executive_t exec;
    nacelle_comms_link_t comms;
    uint16_t sim_battery_voltage;
    uint16_t sim_panel_temp;
    uint16_t sim_gyro_rate;
} flight_system_t;

static flight_system_t g_flight;

static void task_telecommand_ingest(void *ctx) {
    flight_system_t *sys = (flight_system_t *)ctx;
    uint8_t rx_buf[2048];
    size_t rx_len = 0;

    while (nacelle_comms_recv_nonblocking(&sys->comms, rx_buf, sizeof(rx_buf), &rx_len) == NACELLE_OK && rx_len > 0) {
        if (rx_len < 4) {
            continue;
        }

        /* Verify CRC16 if frame contains trailer */
        if (rx_len >= 6) {
            uint16_t expected_crc = nacelle_read_u16_be(&rx_buf[rx_len - 2]);
            if (!nacelle_crc16_verify(rx_buf, rx_len - 2, expected_crc)) {
                continue;
            }
        }

        /* Parse frame header */
        nacelle_frame_header_t frame_hdr;
        frame_hdr.category = (nacelle_frame_category_t)(rx_buf[0] & 0x03);
        frame_hdr.virtual_chan = (uint8_t)((rx_buf[0] >> 2) & 0x3F);
        frame_hdr.frame_seq_num = rx_buf[1];
        frame_hdr.payload_len = (uint16_t)(rx_len - 4);

        bool accepted = false;
        nacelle_receiver_process_frame(&sys->exec.receiver, &frame_hdr, &accepted);

        if (accepted && frame_hdr.payload_len >= NACELLE_PACKET_HEADER_LEN) {
            nacelle_packet_view_t pkt;
            if (nacelle_packet_parse(&rx_buf[2], frame_hdr.payload_len, &pkt) == NACELLE_OK) {
                nacelle_queue_entry_t entry;
                entry.command_id = (uint32_t)sys->exec.cmd_queue.count + 1;
                entry.dispatch_time = nacelle_get_monotonic_time();
                entry.priority = NACELLE_PRIORITY_NORMAL;
                entry.channel = pkt.header.channel;
                entry.opcode = (pkt.payload_len >= 2) ? nacelle_read_u16_be(pkt.payload) : 0;
                entry.arg_len = (uint8_t)((pkt.payload_len > 2) ? (pkt.payload_len - 2) : 0);
                if (entry.arg_len > NACELLE_MAX_CMD_ARG_LEN) {
                    entry.arg_len = NACELLE_MAX_CMD_ARG_LEN;
                }
                if (entry.arg_len > 0) {
                    memcpy(entry.args, &pkt.payload[2], entry.arg_len);
                }
                nacelle_queue_insert(&sys->exec.cmd_queue, &entry);
            }
        }
    }
}

static void task_command_dispatch(void *ctx) {
    flight_system_t *sys = (flight_system_t *)ctx;
    nacelle_timestamp_t now = nacelle_get_monotonic_time();

    while (!nacelle_queue_is_empty(&sys->exec.cmd_queue)) {
        nacelle_queue_entry_t entry;
        if (nacelle_queue_peek(&sys->exec.cmd_queue, &entry) != NACELLE_OK) {
            break;
        }

        if (nacelle_timestamp_compare(entry.dispatch_time, now) <= 0) {
            nacelle_queue_extract_min(&sys->exec.cmd_queue, &entry);
            /* Handle command opcode */
            if (entry.opcode == 0x1001) {
                /* Reset simulation variables */
                sys->sim_battery_voltage = 3300;
                sys->sim_panel_temp = 250;
            } else if (entry.opcode == 0x2001) {
                /* Anomaly injection for testing */
                sys->sim_battery_voltage = 2700;
            }
        } else {
            break;
        }
    }
}

static void task_health_monitoring(void *ctx) {
    flight_system_t *sys = (flight_system_t *)ctx;

    nacelle_recovery_action_t act1 = NACELLE_ACTION_NONE;
    nacelle_recovery_action_t act2 = NACELLE_ACTION_NONE;

    nacelle_fdir_update_channel(&sys->exec.fdir, 1, sys->sim_battery_voltage, &act1);
    nacelle_fdir_update_channel(&sys->exec.fdir, 2, sys->sim_panel_temp, &act2);

    if (act1 == NACELLE_ACTION_SAFE_HOLD || act2 == NACELLE_ACTION_SAFE_HOLD) {
        /* Escalation to safe hold */
        sys->exec.fdir.system_in_safe_hold = true;
    }
}

static void task_telemetry_downlink(void *ctx) {
    flight_system_t *sys = (flight_system_t *)ctx;

    /* Build telemetry data payload (housekeeping metrics) */
    uint8_t hk_payload[64];
    nacelle_timestamp_t now = nacelle_get_monotonic_time();
    nacelle_write_u32_be(&hk_payload[0], now.coarse_secs);
    nacelle_write_u16_be(&hk_payload[4], now.fine_ticks);
    nacelle_write_u16_be(&hk_payload[6], sys->sim_battery_voltage);
    nacelle_write_u16_be(&hk_payload[8], sys->sim_panel_temp);
    nacelle_write_u16_be(&hk_payload[10], sys->sim_gyro_rate);
    hk_payload[12] = (uint8_t)sys->exec.fdir.system_in_safe_hold;
    hk_payload[13] = sys->exec.receiver.expected_seq;
    nacelle_write_u16_be(&hk_payload[14], (uint16_t)sys->exec.cmd_queue.count);

    /* Generate link status report */
    nacelle_link_report_t report;
    nacelle_generate_link_report(&sys->exec.receiver, 0, &report);

    /* Construct 1024-byte telemetry transfer frame */
    uint8_t tm_frame[NACELLE_TM_FRAME_SIZE];
    nacelle_status_t st = nacelle_tm_build_frame(&sys->exec.tm_mux, 0, 0,
                                                 hk_payload, 16,
                                                 &report, tm_frame);
    if (st == NACELLE_OK) {
        nacelle_ring_buffer_write(&sys->exec.telemetry_log, tm_frame, 64);
        if (sys->comms.has_remote) {
            nacelle_comms_send(&sys->comms, tm_frame, NACELLE_TM_FRAME_SIZE);
        }
    }
}

int main(int argc, char **argv) {
    uint16_t port = 8088;
    int max_cycles = -1;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = (uint16_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--cycles") == 0 && i + 1 < argc) {
            max_cycles = atoi(argv[++i]);
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    flight_system_t *sys = &g_flight;
    nacelle_executive_init(&sys->exec, 42);

    sys->sim_battery_voltage = 3300; /* 3300 mV (3.3V) nominal */
    sys->sim_panel_temp = 250;       /* 25.0 C nominal */
    sys->sim_gyro_rate = 10;         /* 0.1 deg/s */

    /* Register FDIR health monitoring channels */
    nacelle_channel_thresholds_t batt_thresh = {
        .warn_low = 3000, .warn_high = 3600,
        .crit_low = 2800, .crit_high = 3800
    };
    nacelle_channel_thresholds_t temp_thresh = {
        .warn_low = 0,    .warn_high = 600,
        .crit_low = 65436,.crit_high = 800
    };
    nacelle_fdir_register_channel(&sys->exec.fdir, 1, 3, &batt_thresh);
    nacelle_fdir_register_channel(&sys->exec.fdir, 2, 3, &temp_thresh);

    /* Initialize socket comms */
    nacelle_comms_init(&sys->comms, port);

    /* Register cyclic executive tasks */
    nacelle_executive_register_task(&sys->exec, "tc_ingest", task_telecommand_ingest, sys, 1, 20);
    nacelle_executive_register_task(&sys->exec, "cmd_dispatch", task_command_dispatch, sys, 1, 10);
    nacelle_executive_register_task(&sys->exec, "health_monitor", task_health_monitoring, sys, 2, 15);
    nacelle_executive_register_task(&sys->exec, "tm_downlink", task_telemetry_downlink, sys, 5, 30);

    printf("Nacelle flight executive initialized on UDP port %u\n", port);
    printf("Spacecraft ID: 42, Major cycle: 1000ms, Minor cycle: 100ms\n");

    int cycles_run = 0;
    while (s_keep_running) {
        nacelle_executive_step_minor(&sys->exec);
        cycles_run++;

        if (max_cycles > 0 && cycles_run >= max_cycles) {
            break;
        }

        /* 100ms minor cycle pacing (or fast-forward if running test cycle mode) */
        if (max_cycles <= 0) {
            usleep(100000); /* 100,000 microseconds = 100ms */
        }
    }

    printf("Shutdown complete. Total minor cycles: %d, Dispatches: %u\n",
           cycles_run, sys->exec.total_dispatches);

    nacelle_comms_close(&sys->comms);
    return 0;
}
