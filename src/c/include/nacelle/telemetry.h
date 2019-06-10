#ifndef NACELLE_TELEMETRY_H
#define NACELLE_TELEMETRY_H

#include "nacelle/types.h"
#include "nacelle/sequence.h"

#define NACELLE_TM_FRAME_SIZE       1024
#define NACELLE_TM_HEADER_SIZE      6
#define NACELLE_TM_REPORT_SIZE      4
#define NACELLE_TM_CRC_SIZE         2
#define NACELLE_TM_DATA_FIELD_SIZE  (NACELLE_TM_FRAME_SIZE - NACELLE_TM_HEADER_SIZE - NACELLE_TM_REPORT_SIZE - NACELLE_TM_CRC_SIZE)

#define NACELLE_FHP_NO_FIRST_HEADER 0x07FEU
#define NACELLE_FHP_IDLE_DATA_ONLY  0x07FFU

typedef struct {
    uint8_t version_num;
    uint16_t spacecraft_id;
    uint8_t virtual_channel;
    bool ocf_flag;
    uint8_t master_frame_seq;
    uint8_t virt_frame_seq;
    uint16_t first_header_ptr;
} nacelle_tm_header_t;

typedef struct {
    uint8_t virtual_channel;
    uint8_t next_virt_seq;
} nacelle_vc_state_t;

typedef struct {
    uint16_t spacecraft_id;
    uint8_t next_master_seq;
    nacelle_vc_state_t vc_states[8];
} nacelle_multiplexer_t;

void nacelle_multiplexer_init(nacelle_multiplexer_t *mux, uint16_t scid);
nacelle_status_t nacelle_tm_build_frame(nacelle_multiplexer_t *mux,
                                        uint8_t vcid,
                                        uint16_t first_header_ptr,
                                        const uint8_t *data,
                                        size_t data_len,
                                        const nacelle_link_report_t *report,
                                        uint8_t *out_frame);

size_t nacelle_generate_idle_pattern(uint8_t *buffer, size_t length);

#endif /* NACELLE_TELEMETRY_H */
