#include "onion/transport.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static onion_status transfer(int fd, void *data, size_t size, bool sending) {
    size_t offset = 0;
    while (offset < size) {
        ssize_t result = sending
            ? send(fd, (const char *)data + offset, size - offset, 0)
            : recv(fd, (char *)data + offset, size - offset, 0);
        if (result == 0) {
            return ONION_E_IO;
        }
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return ONION_E_IO;
        }
        offset += (size_t)result;
    }
    return ONION_OK;
}

onion_status onion_transport_send_frame(const onion_transport *transport,
                                        const onion_plugin_ipc_frame *frame) {
    if (!transport || !transport->send || !frame ||
        frame->magic != ONION_PLUGIN_IPC_MAGIC ||
        frame->version != ONION_PLUGIN_IPC_VERSION ||
        frame->payload_size > ONION_PLUGIN_IPC_MAX_PAYLOAD) {
        return ONION_E_INVALID_ARGUMENT;
    }
    return transport->send(transport->context, frame, sizeof(*frame));
}

onion_status onion_transport_recv_frame(const onion_transport *transport,
                                        onion_plugin_ipc_frame *frame) {
    if (!transport || !transport->recv || !frame) {
        return ONION_E_INVALID_ARGUMENT;
    }
    onion_status result = transport->recv(transport->context, frame, sizeof(*frame));
    if (result != ONION_OK) {
        return result;
    }
    if (frame->magic != ONION_PLUGIN_IPC_MAGIC ||
        frame->version != ONION_PLUGIN_IPC_VERSION ||
        frame->payload_size > ONION_PLUGIN_IPC_MAX_PAYLOAD) {
        return ONION_E_PROTOCOL;
    }
    return ONION_OK;
}

static onion_status socket_send(void *context, const void *data, size_t size) {
    onion_socket_transport *state = (onion_socket_transport *)context;
    return transfer(state->fd, (void *)data, size, true);
}

static onion_status socket_recv(void *context, void *data, size_t size) {
    onion_socket_transport *state = (onion_socket_transport *)context;
    return transfer(state->fd, data, size, false);
}

static void socket_close(void *context) {
    onion_socket_transport *state = (onion_socket_transport *)context;
    if (state && state->owns_fd && state->fd >= 0) {
        close(state->fd);
        state->fd = -1;
    }
}

onion_status onion_socket_transport_init(onion_transport *transport,
                                         onion_socket_transport *state,
                                         int fd, bool owns_fd) {
    if (!transport || !state || fd < 0) {
        return ONION_E_INVALID_ARGUMENT;
    }
    state->fd = fd;
    state->owns_fd = owns_fd;
    transport->context = state;
    transport->send = socket_send;
    transport->recv = socket_recv;
    transport->close = socket_close;
    return ONION_OK;
}

void onion_socket_transport_deinit(onion_transport *transport) {
    if (!transport) {
        return;
    }
    if (transport->close) {
        transport->close(transport->context);
    }
    memset(transport, 0, sizeof(*transport));
}

