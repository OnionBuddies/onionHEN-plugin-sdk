#include "onion/client.h"

#include <errno.h>
#include <string.h>

static onion_status client_request(onion_client *client, uint16_t command,
                                   const void *payload, size_t payload_size,
                                   onion_plugin_ipc_frame *response) {
    if (!client || !client->initialized || !client->transport || !response ||
        payload_size > ONION_PLUGIN_IPC_MAX_PAYLOAD) return ONION_E_NOT_INITIALIZED;
    onion_plugin_ipc_frame request;
    memset(&request, 0, sizeof(request));
    request.magic = ONION_PLUGIN_IPC_MAGIC;
    request.version = ONION_PLUGIN_IPC_VERSION;
    request.command = command;
    request.request_id = client->next_request_id++;
    request.payload_size = (uint32_t)payload_size;
    if (payload_size) memcpy(request.payload, payload, payload_size);
    pthread_mutex_lock(&client->mutex);
    onion_status result = onion_transport_send_frame(client->transport, &request);
    if (result == ONION_OK) result = onion_transport_recv_frame(client->transport, response);
    pthread_mutex_unlock(&client->mutex);
    if (result != ONION_OK) return result;
    if (response->command != ONION_PLUGIN_IPC_RESPONSE ||
        response->request_id != request.request_id || response->payload_size < 8) {
        return ONION_E_PROTOCOL;
    }
    return ONION_OK;
}

static onion_status response_status(const onion_plugin_ipc_frame *response,
                                    const uint8_t **data, size_t *size) {
    if (!response || !data || !size || response->payload_size < 8) return ONION_E_PROTOCOL;
    int32_t status;
    uint32_t data_size;
    memcpy(&status, response->payload, sizeof(status));
    memcpy(&data_size, response->payload + 4, sizeof(data_size));
    if ((size_t)data_size > response->payload_size - 8) return ONION_E_PROTOCOL;
    *data = response->payload + 8;
    *size = data_size;
    return (onion_status)status;
}

static onion_status service_log(void *context, onion_log_level level, const char *message) {
    uint32_t level_value = (uint32_t)level;
    size_t length = message ? strlen(message) : 0;
    if (length + sizeof(level_value) > ONION_PLUGIN_IPC_MAX_PAYLOAD) return ONION_E_INVALID_ARGUMENT;
    uint8_t payload[ONION_PLUGIN_IPC_MAX_PAYLOAD];
    memcpy(payload, &level_value, sizeof(level_value));
    if (length) memcpy(payload + sizeof(level_value), message, length);
    onion_plugin_ipc_frame response;
    onion_status result = client_request((onion_client *)context, ONION_PLUGIN_IPC_LOG,
                                         payload, sizeof(level_value) + length, &response);
    if (result != ONION_OK) return result;
    const uint8_t *data; size_t size;
    return response_status(&response, &data, &size);
}

static onion_status service_notify(void *context, const char *message) {
    onion_plugin_ipc_frame response;
    size_t length = message ? strlen(message) : 0;
    if (!message || length >= ONION_PLUGIN_IPC_MAX_PAYLOAD) return ONION_E_INVALID_ARGUMENT;
    onion_status result = client_request((onion_client *)context, ONION_PLUGIN_IPC_NOTIFY,
                                         message, length, &response);
    if (result != ONION_OK) return result;
    const uint8_t *data; size_t size;
    return response_status(&response, &data, &size);
}

static onion_status service_config_get(void *context, const char *key,
                                       char *value, size_t value_size) {
    onion_plugin_ipc_frame response;
    if (!key || !value || value_size == 0) return ONION_E_INVALID_ARGUMENT;
    size_t key_size = strlen(key) + 1;
    if (key_size > ONION_PLUGIN_IPC_MAX_PAYLOAD) return ONION_E_INVALID_ARGUMENT;
    onion_status result = client_request((onion_client *)context, ONION_PLUGIN_IPC_CONFIG_GET,
                                         key, key_size, &response);
    if (result != ONION_OK) return result;
    const uint8_t *data; size_t size;
    result = response_status(&response, &data, &size);
    if (result != ONION_OK) return result;
    if (size >= value_size) return ONION_E_INVALID_ARGUMENT;
    memcpy(value, data, size);
    value[size] = '\0';
    return ONION_OK;
}

static onion_status service_config_set(void *context, const char *key, const char *value) {
    onion_plugin_ipc_frame response;
    if (!key || !value) return ONION_E_INVALID_ARGUMENT;
    size_t key_size = strlen(key) + 1;
    size_t value_size = strlen(value) + 1;
    if (key_size + value_size > ONION_PLUGIN_IPC_MAX_PAYLOAD) return ONION_E_INVALID_ARGUMENT;
    uint8_t payload[ONION_PLUGIN_IPC_MAX_PAYLOAD];
    memcpy(payload, key, key_size);
    memcpy(payload + key_size, value, value_size);
    onion_status result = client_request((onion_client *)context, ONION_PLUGIN_IPC_CONFIG_SET,
                                         payload, key_size + value_size, &response);
    if (result != ONION_OK) return result;
    const uint8_t *data; size_t size;
    return response_status(&response, &data, &size);
}

onion_status onion_client_init(onion_client *client, onion_transport *transport) {
    if (!client || !transport || !transport->send || !transport->recv) return ONION_E_INVALID_ARGUMENT;
    memset(client, 0, sizeof(*client));
    if (pthread_mutex_init(&client->mutex, NULL) != 0) return ONION_E_IO;
    client->transport = transport;
    client->next_request_id = 1;
    client->initialized = 1;
    return ONION_OK;
}

void onion_client_deinit(onion_client *client) {
    if (!client || !client->initialized) return;
    pthread_mutex_destroy(&client->mutex);
    client->transport = NULL;
    client->initialized = 0;
}

onion_status onion_client_make_services(onion_client *client,
                                        onion_host_services_v1 *out_services) {
    if (!client || !client->initialized || !out_services) return ONION_E_INVALID_ARGUMENT;
    memset(out_services, 0, sizeof(*out_services));
    out_services->struct_size = sizeof(*out_services);
    out_services->abi_version = ONION_HOST_SERVICES_ABI_VERSION;
    out_services->context = client;
    out_services->log = service_log;
    out_services->notify = service_notify;
    out_services->config_get = service_config_get;
    out_services->config_set = service_config_set;
    return ONION_OK;
}

onion_status onion_client_ping(onion_client *client) {
    onion_plugin_ipc_frame response;
    onion_status result = client_request(client, ONION_PLUGIN_IPC_PING, NULL, 0, &response);
    if (result != ONION_OK) return result;
    const uint8_t *data; size_t size;
    return response_status(&response, &data, &size);
}
