#pragma once

#include <stdint.h>

#define ONION_PLUGIN_IPC_MAGIC 0x4F504943u /* 'OPIC' */
#define ONION_PLUGIN_IPC_VERSION 1u
#define ONION_PLUGIN_IPC_MAX_PAYLOAD 4096u

enum onion_plugin_ipc_command {
    ONION_PLUGIN_IPC_PING = 1,
    ONION_PLUGIN_IPC_RESPONSE = 2,
    ONION_PLUGIN_IPC_LOG = 3,
    ONION_PLUGIN_IPC_NOTIFY = 4,
    ONION_PLUGIN_IPC_CONFIG_GET = 5,
    ONION_PLUGIN_IPC_CONFIG_SET = 6,
    ONION_PLUGIN_IPC_GET_STATUS = 7,
    ONION_PLUGIN_IPC_STOP = 8,
    ONION_PLUGIN_IPC_EVENT = 9
};

typedef struct onion_plugin_ipc_response {
    int32_t status;
    uint32_t data_size;
    uint8_t data[];
} onion_plugin_ipc_response;

/* Wire format is intentionally fixed-width. Do not put pointers or C++ types here. */
typedef struct onion_plugin_ipc_frame {
    uint32_t magic;
    uint16_t version;
    uint16_t command;
    uint32_t request_id;
    uint32_t payload_size;
    uint8_t payload[ONION_PLUGIN_IPC_MAX_PAYLOAD];
} onion_plugin_ipc_frame;

#define ONION_PLUGIN_IPC_RESPONSE_HEADER_SIZE 8u
