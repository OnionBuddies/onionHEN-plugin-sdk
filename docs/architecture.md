# SDK Architecture

The SDK is organized around dependency inversion and a small C ABI.

```text
plugin business code
        |
        v
onion_plugin_runtime  ---- onion_host_services_v1
        |                         |
        v                         v
    event_bus              IPC client adapter
                                  |
                                  v
                         onion_transport
                                  |
                 Unix socket / mock / future transport
```

## Responsibilities

| Component | Responsibility | Stable boundary |
| --- | --- | --- |
| `include/onion/*.h` | C ABI, wire structures, error codes | Yes |
| `runtime/status.c` | Error-to-string mapping | Yes |
| `runtime/runtime.c` | Lifecycle state machine and callbacks | Yes |
| `runtime/services.c` | Service validation and typed config helpers | Yes |
| `runtime/event_bus.c` | Thread-safe subscribe/publish | Yes |
| `runtime/transport.c` | Complete frame transport | Yes |
| `runtime/client.c` | IPC-backed Host Services adapter | Yes |
| `runtime/host_api.c` | Legacy fd convenience wrappers | Compatibility |

Plugin code should depend on interfaces (`onion_host_services_v1`,
`onion_transport`, `onion_event_bus`) rather than Unix sockets, OnionHEN daemon
internals, or C++ implementation types. A host can therefore replace the
transport or provide a test double without rebuilding plugin business logic.

## ABI rules

- All public structures begin with `struct_size` and `abi_version` where they
  can be extended.
- Wire messages use fixed-width integers and contain no pointers.
- The current IPC wire representation is little-endian, matching the PS5
  target; a future cross-endian transport must serialize each integer field.
- C++ consumers must include the headers inside `extern "C"`; STL objects and
  exceptions never cross the boundary.
- Callbacks run outside the event bus mutex. A callback may unsubscribe itself
  or publish another event.
- Host services are capability-scoped. A missing function pointer is a denied
  capability, not an invitation to call a private host symbol.

## Lifecycle

```text
CREATED -> INITIALIZED -> RUNNING -> STOPPING -> STOPPED -> SHUTDOWN
                    \-> FAILED -> STOPPING -> STOPPED
```

`on_init` is called after descriptor and host ABI validation. `on_start` and
`on_stop` are called only during valid state transitions. A callback failure
moves the runtime to `FAILED` and returns the callback's error code.

## Current scope

This release implements the SDK-side contracts and client runtime. OnionHEN
host-side plugin discovery, IPC broker, UI registry, and ShellUI XML adapter
are intentionally separate projects. Keeping that boundary prevents a plugin
from depending on an unstable ShellUI or firmware-specific implementation.
