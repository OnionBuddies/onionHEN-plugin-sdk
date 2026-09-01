#pragma once

#include <pthread.h>
#include <stdint.h>

#include "onion/services.h"
#include "onion/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct onion_client {
    onion_transport *transport;
    pthread_mutex_t mutex;
    uint32_t next_request_id;
    int initialized;
} onion_client;

#define ONION_CLIENT_INITIALIZER {0}

onion_status onion_client_init(onion_client *client, onion_transport *transport);
void onion_client_deinit(onion_client *client);
onion_status onion_client_make_services(onion_client *client,
                                        onion_host_services_v1 *out_services);
onion_status onion_client_ping(onion_client *client);

#ifdef __cplusplus
}
#endif
