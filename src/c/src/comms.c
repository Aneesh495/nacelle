#include "nacelle/comms.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

nacelle_status_t nacelle_comms_init(nacelle_comms_link_t *link, uint16_t listen_port) {
    if (!link) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    memset(link, 0, sizeof(*link));
    link->port = listen_port;
    link->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (link->sockfd < 0) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    /* Set socket to non-blocking */
    int flags = fcntl(link->sockfd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(link->sockfd, F_SETFL, flags | O_NONBLOCK);
    }

    /* Allow address reuse */
    int opt = 1;
    setsockopt(link->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    link->bind_addr.sin_family = AF_INET;
    link->bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    link->bind_addr.sin_port = htons(listen_port);

    if (bind(link->sockfd, (struct sockaddr *)&link->bind_addr, sizeof(link->bind_addr)) < 0) {
        close(link->sockfd);
        link->sockfd = -1;
        return NACELLE_ERR_INVALID_PARAM;
    }

    return NACELLE_OK;
}

void nacelle_comms_close(nacelle_comms_link_t *link) {
    if (link && link->sockfd >= 0) {
        close(link->sockfd);
        link->sockfd = -1;
    }
}

nacelle_status_t nacelle_comms_recv_nonblocking(nacelle_comms_link_t *link,
                                                uint8_t *buffer,
                                                size_t max_len,
                                                size_t *out_received) {
    if (!link || !buffer || link->sockfd < 0) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    ssize_t n = recvfrom(link->sockfd, buffer, max_len, 0,
                         (struct sockaddr *)&client_addr, &addr_len);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (out_received) *out_received = 0;
            return NACELLE_OK;
        }
        return NACELLE_ERR_INVALID_PARAM;
    }

    /* Record sender address for response */
    link->remote_addr = client_addr;
    link->has_remote = true;
    link->packets_received++;
    link->bytes_received += (uint32_t)n;

    if (out_received) {
        *out_received = (size_t)n;
    }

    return NACELLE_OK;
}

nacelle_status_t nacelle_comms_send(nacelle_comms_link_t *link,
                                    const uint8_t *data,
                                    size_t length) {
    if (!link || !data || link->sockfd < 0 || !link->has_remote) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    ssize_t n = sendto(link->sockfd, data, length, 0,
                       (const struct sockaddr *)&link->remote_addr,
                       sizeof(link->remote_addr));

    if (n < 0) {
        return NACELLE_ERR_INVALID_PARAM;
    }

    link->packets_transmitted++;
    link->bytes_transmitted += (uint32_t)n;
    return NACELLE_OK;
}
