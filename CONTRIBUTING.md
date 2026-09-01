# Contributing

Thank you for helping improve OnionHEN Plugin SDK. Keep changes focused and put
them in the layer that owns the responsibility. New public API is a long-term
compatibility commitment, so discuss substantial ABI or protocol changes in a
feature request before implementing them.

All participation is governed by the [Code of Conduct](CODE_OF_CONDUCT.md).

## Development setup

Host-side development does not require a PS5 SDK:

```sh
cmake -S . -B build -G Ninja -DONION_SDK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

To build PS5 samples:

```sh
export PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk
cmake -S . -B build-ps5 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ps5-toolchain.cmake \
  -DONION_SDK_BUILD_SAMPLES=ON \
  -DPS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK"
cmake --build build-ps5
```

Do not commit `build/`, `build-ps5/`, generated artifacts, Python bytecode, PS5
SDK files, proprietary libraries, decrypted system files, keys, or console
dumps.

## Where changes belong

| Change | Location |
| --- | --- |
| Stable public C ABI | `include/onion/` |
| Lifecycle and service implementation | `runtime/` |
| CMake consumer API/toolchain | `cmake/` |
| Packaging and inspection utilities | `tools/` |
| Plugin examples | `samples/` |
| Host-side tests | `tests/` |
| Architecture and protocol rationale | `docs/` |

Read [docs/architecture.md](docs/architecture.md) before changing a public
structure, lifecycle transition, service interface, transport, or wire frame.

## Design rules

- Keep the public ABI in C. Do not expose C++ classes, STL types, exceptions,
  compiler-specific RTTI, or host function addresses.
- Public extensible structures start with `struct_size` and `abi_version`.
- Wire structures use fixed-width integer types and contain no pointers.
- Plugin business code depends on service/transport interfaces, not daemon or
  socket implementation details.
- Keep modules single-purpose. Extend an existing owner before creating a
  parallel implementation.
- Return `onion_status` from fallible SDK APIs and preserve specific errors.
- Add focused host tests for state transitions, malformed input, and callbacks.
- Document any compatibility decision that is not evident from the code.

## Public ABI changes

Before changing a public header:

1. Explain the use case and compatibility impact in an issue.
2. Prefer adding an optional service or a new versioned structure.
3. Never reorder, remove, or reinterpret released fields in-place.
4. Update the ABI version only when old consumers cannot safely interoperate.
5. Update `docs/architecture.md`, samples, and tests in the same pull request.

The wire protocol currently targets little-endian PS5 processes. A transport
that crosses architectures must serialize fields explicitly rather than copy
native structures directly.

## Pull requests

- Keep one logical change per pull request.
- Explain what changed, why it belongs in the SDK, and the compatibility impact.
- Include host test results and PS5 build/hardware results when available.
- Update both READMEs when user-facing setup or supported capabilities change.
- Do not mix formatting or unrelated refactors into a behavior change.
- Contributions are licensed under the repository's [GNU GPL v3](LICENSE).

Maintainers may ask for a smaller change, additional tests, or an ABI proposal
before accepting a new public API.
