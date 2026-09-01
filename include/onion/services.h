#pragma once

#include <stddef.h>
#include <stdint.h>

#include "onion/host_api.h"
#include "onion/status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ONION_HOST_SERVICES_ABI_VERSION 1u

typedef onion_status (*onion_service_log_fn)(void *context,
                                             onion_log_level level,
                                             const char *message);
typedef onion_status (*onion_service_notify_fn)(void *context,
                                                const char *message);
typedef onion_status (*onion_service_config_get_fn)(void *context,
                                                    const char *key,
                                                    char *value,
                                                    size_t value_size);
typedef onion_status (*onion_service_config_set_fn)(void *context,
                                                    const char *key,
                                                    const char *value);

typedef struct onion_host_services_v1 {
    uint32_t struct_size;
    uint32_t abi_version;
    void *context;
    onion_service_log_fn log;
    onion_service_notify_fn notify;
    onion_service_config_get_fn config_get;
    onion_service_config_set_fn config_set;
} onion_host_services_v1;

onion_status onion_services_validate(const onion_host_services_v1 *services);
onion_status onion_service_log(const onion_host_services_v1 *services,
                               onion_log_level level, const char *message);
onion_status onion_service_notify(const onion_host_services_v1 *services,
                                  const char *message);
onion_status onion_service_config_get(const onion_host_services_v1 *services,
                                      const char *key, char *value,
                                      size_t value_size);
onion_status onion_service_config_set(const onion_host_services_v1 *services,
                                      const char *key, const char *value);
onion_status onion_service_config_get_int(const onion_host_services_v1 *services,
                                          const char *key, int64_t *value);
onion_status onion_service_config_set_int(const onion_host_services_v1 *services,
                                          const char *key, int64_t value);
onion_status onion_service_config_get_bool(const onion_host_services_v1 *services,
                                           const char *key, int *value);
onion_status onion_service_config_set_bool(const onion_host_services_v1 *services,
                                           const char *key, int value);

#ifdef __cplusplus
}
#endif
