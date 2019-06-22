#ifndef NACELLE_COMMS_H
#define NACELLE_COMMS_H

#include "nacelle/types.h"
#include <netinet/in.h>

typedef struct {
    int sockfd;
    struct sockaddr_in bind_addr;
    struct sockaddr_in remote_addr;
    bool has_remote;
    uint16_t port;
    uint32_t packets_received;
    uint32_t packets_transmitted;
    uint32_t bytes_received;
    uint32_t bytes_transmitted;
} nacelle_comms_link_t;

nacelle_status_t nacelle_comms_init(nacelle_comms_link_t *link, uint16_t listen_port);
void nacelle_comms_close(nacelle_comms_link_t *link);

nacelle_status_t nacelle_comms_recv_nonblocking(nacelle_comms_link_t *link,
                                                uint8_t *buffer,
                                                size_t max_len,
                                                size_t *out_received);

nacelle_status_t nacelle_comms_send(nacelle_comms_link_t *link,
                                    const uint8_t *data,
                                    size_t length);

#endif /* NACELLE_COMMS_H */
