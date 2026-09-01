<p align="center">
  <img src="assets/logo.png" alt="OnionHEN" height="128" width="128"/>
</p>

<p align="center">
  <b>OnionHEN Plugin SDK</b><br/>
  A modular C SDK for independent PlayStation 5 payload plugins
</p>

<p align="center">
  <a href="README_ZH.md">简体中文</a>
  ·
  <b>English</b>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="license"/></a>
  <img src="https://img.shields.io/badge/Platform-PlayStation%205-003791?style=flat&logo=playstation" alt="PlayStation 5"/>
  <img src="https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white" alt="C"/>
  <img src="https://img.shields.io/badge/Build-CMake-064F8C?style=flat&logo=cmake" alt="CMake"/>
</p>

OnionHEN Plugin SDK provides the contracts and small runtime needed to build
standalone PS5 ELF plugins. Plugins run as independent processes and use a
versioned C ABI, host services, lifecycle callbacks, events, and a replaceable
IPC transport. The SDK is designed to evolve without exposing OnionHEN daemon
internals or a C++ ABI to plugin authors.

> This repository is the SDK side of the integration. OnionHEN host-side
> discovery, plugin management, and ShellUI/WebUI backends are separate work.
> The current OnionHEN host loads bare `.elf` payloads through its private
> loader; the SDK's `.opk` file is only a distribution format for now.

## Features

- CMake `onion_add_plugin(...)` helper for PS5 targets
- Stable, versioned plugin descriptor embedded in `.onion_plugin`
- Explicit lifecycle state machine and callbacks
- Capability-scoped host services for logging, notifications, and configuration
- Typed configuration helpers for strings, integers, and booleans
- Thread-safe event bus with safe unsubscribe during callbacks
- Fixed-width IPC frames with request IDs and status responses
- Replaceable transport interface for sockets, mocks, or future transports
- Python tools to package, inspect, and deploy plugin artifacts
- Minimal `hello` and daemon samples

## Requirements

- PS5 Payload SDK (`PS5_PAYLOAD_SDK`)
- CMake 3.20 or newer
- Ninja (recommended)
- Clang/LLVM with the Prospero target
- Python 3.9 or newer

Host-side Runtime tests only require a C compiler, CMake, Python, and pthreads.

## Build and test

Build the host-side runtime tests:

```sh
cmake -S . -B build -G Ninja -DONION_SDK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Build the PS5 samples:

```sh
export PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk
cmake -S . -B build-ps5 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ps5-toolchain.cmake \
  -DONION_SDK_BUILD_SAMPLES=ON \
  -DPS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK"
cmake --build build-ps5 --target hello_package
```

The ELF is written to `build-ps5/bin/hello.elf`. The optional package is written
to `build-ps5/packages/hello.opk`.

## Create a plugin

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyPlugin C)

find_package(OnionHENPluginSDK CONFIG REQUIRED)
onion_add_plugin(
    NAME my_plugin
    TITLE_ID MYPL00001
    VERSION 1.00
    SOURCES source/main.c
    LIBRARIES SceLibcInternal kernel_sys)
```

`onion_add_plugin` automatically links `OnionHEN::Runtime`. The `main(void)`
entry point is still an ordinary PS5 payload entry point. Keep all data crossing
the host boundary in the public C headers; pointers, STL objects, exceptions,
and C++ class layouts are not part of the ABI.

## Architecture

```text
plugin code
    -> runtime + host services
    -> IPC client adapter
    -> transport (socket, mock, or future implementation)
```

Read [docs/architecture.md](docs/architecture.md) for ownership boundaries,
state transitions, ABI rules, and extension guidance.

## Current scope and roadmap

The current release focuses on the plugin-side foundation. The next host-facing
layers are planned in this order:

1. OnionHEN plugin manager and manifest validation
2. Host IPC broker and capability enforcement
3. Plugin status, auto-start, stop/restart, and crash recovery
4. UI Contribution API with a WebUI backend
5. ShellUI XML backend with firmware-specific adapters
6. Optional etaHEN `.plugin` compatibility tooling

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.
Bug reports and feature requests use the GitHub issue forms. Security-sensitive
reports should follow [SECURITY.md](SECURITY.md).

## Credits and related projects

- [OnionHEN](https://github.com/aydencharles/onionHEN) — the host project and
  original logo used by this repository
- [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk) — Prospero toolchain
  and target headers
- [etaHEN-Plugins](https://github.com/etaHEN/etaHEN-Plugins) — reference for PS5
  plugin project layout and packaging conventions

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
Third-party components retain their respective licenses and notices.

> OnionHEN is an unofficial homebrew project and is not affiliated with Sony
> Interactive Entertainment. Use software from this repository only on
> hardware you own and at your own risk. No warranty is provided.
