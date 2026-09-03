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
             daemon Unix socket / mock / future transport
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
| `runtime/ui.c` | UI document builder, validation, and encoder | Yes |
| `runtime/ui_client.c` | Chunked UI registration client | Yes |
| `runtime/ui_event.c` | Strict UI action event decoder and polling API | Yes |
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

## UI contribution boundary

`onion.ui` is discovered with the optional `query_interface` tail field on
`onion_host_services_v1`. This keeps the original Host Services prefix ABI
compatible and avoids turning unrelated services into one large interface.

```text
plugin descriptor -> opaque document builder -> little-endian document
  -> HELLO-bound connection -> begin / ordered chunks / checksum commit
  -> owner-isolated host registry
  -> immutable snapshot -> firmware-specific ShellUI XML adapter
```

The neutral document owns no ShellUI XML and contains no host pointers. The
host checks the full graph and every value before publishing a snapshot.
Registration is transactional: incomplete or bad-checksum transfers never
replace a visible contribution. `HELLO` binds one self-declared plugin ID and
capability set to the connection for its lifetime; a second `HELLO` or a
duplicate active ID is rejected. UI commands use this bound owner rather than
the document's `plugin_id`, and disconnect removes its transfers and visible
contributions.

This is a cooperative homebrew contract, not an authentication mechanism.
It prevents accidental identity changes and plugin-ID collisions, but it does
not verify a signature, executable, PID, or launch source. Capabilities provide
feature negotiation and host-side API gating; they are not a security boundary
against a malicious plugin.

The production socket is `/system_tmp/onionhen/ipc/plugin_service`.
`onion_socket_transport_connect()` owns the connected descriptor and the daemon
creates one `ConnectionSession` per accepted stream. Listener recovery closes
active streams so plugins reconnect with a fresh identity after rest mode.
The endpoint handles `HELLO`, `PING`, event polling command 9, and UI commands
10–15; the remaining Host Service commands are reserved for their daemon
handlers.

UI action delivery uses `ONION_EVENT_UI_ACTION` and `onion_ui_event_v1`. The SDK
exposes `onion_client_poll_ui_event()` as an explicit non-blocking poll on the
same request/response connection. `ONION_E_NOT_FOUND` means that the owning
plugin currently has no queued action. The daemon derives owner, contribution,
page, and node metadata from its registry; none of these fields are trusted from
ShellUI. Snapshot publication and ShellUI actions use a separate daemon stream
so asynchronous UI traffic cannot be mistaken for plugin IPC responses.

## Lifecycle

```text
CREATED -> INITIALIZED -> RUNNING -> STOPPING -> STOPPED -> SHUTDOWN
                    \-> FAILED -> STOPPING -> STOPPED
```

`on_init` is called after descriptor and host ABI validation. `on_start` and
`on_stop` are called only during valid state transitions. A callback failure
moves the runtime to `FAILED` and returns the callback's error code.

## Current scope

This release implements the SDK-side contracts and client runtime. OnionHEN now
has the host registry, protocol dispatcher, cooperative connection-session
core, daemon plugin socket, cross-process ShellUI bridge, XML adapter, owner
event queues, and an ELF plugin manager.
Plugins are discovered as standard `.elf` files under
`/data/OnionHEN/plugins/`; their `.onion_plugin` descriptor is validated before
the private loader starts an auto-start instance. Remaining log, notification,
and configuration Host Service handlers remain future host work.
