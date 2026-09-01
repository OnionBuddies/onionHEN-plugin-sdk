<p align="center">
  <img src="assets/logo.png" alt="OnionHEN" height="128" width="128"/>
</p>

<p align="center">
  <b>OnionHEN Plugin SDK</b><br/>
  面向 PlayStation 5 独立 Payload 插件的模块化 C SDK
</p>

<p align="center">
  <b>简体中文</b>
  ·
  <a href="README.md">English</a>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="许可证"/></a>
  <img src="https://img.shields.io/badge/Platform-PlayStation%205-003791?style=flat&logo=playstation" alt="PlayStation 5"/>
  <img src="https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white" alt="C"/>
  <img src="https://img.shields.io/badge/Build-CMake-064F8C?style=flat&logo=cmake" alt="CMake"/>
</p>

OnionHEN Plugin SDK 为独立运行的 PS5 ELF 插件提供基础协议和运行时。
插件以独立进程运行，通过版本化 C ABI、宿主服务、生命周期回调、事件总线
和可替换 IPC 传输与宿主通信。SDK 不暴露 OnionHEN daemon 内部实现，也不依赖
C++ ABI，方便后续演进和测试。

> 本仓库实现的是插件侧 SDK。OnionHEN 宿主侧的插件扫描、管理以及
> ShellUI/WebUI 后端属于独立工作。当前 OnionHEN 宿主通过私有 loader 加载
> 裸 `.elf`；SDK 的 `.opk` 目前只是分发格式。

## 功能

- 面向 PS5 target 的 CMake `onion_add_plugin(...)`
- 写入 `.onion_plugin` section 的版本化插件描述符
- 明确的生命周期状态机和回调
- 日志、通知、配置等 capability-scoped 宿主服务
- 字符串、整数、布尔配置辅助函数
- 支持回调中安全取消订阅的线程安全事件总线
- 带 request ID 和状态响应的固定宽度 IPC frame
- 可替换 transport，支持 socket、mock 以及未来实现
- 插件打包、检查和部署 Python 工具
- `hello` 与 daemon 最小示例

## 依赖

- PS5 Payload SDK（`PS5_PAYLOAD_SDK`）
- CMake 3.20 或更高版本
- Ninja（推荐）
- 支持 Prospero target 的 Clang/LLVM
- Python 3.9 或更高版本

主机侧 Runtime 测试只需要 C 编译器、CMake、Python 和 pthreads。

## 构建与测试

构建主机侧 Runtime 测试：

```sh
cmake -S . -B build -G Ninja -DONION_SDK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

构建 PS5 示例：

```sh
export PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk
cmake -S . -B build-ps5 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ps5-toolchain.cmake \
  -DONION_SDK_BUILD_SAMPLES=ON \
  -DPS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK"
cmake --build build-ps5 --target hello_package
```

ELF 输出到 `build-ps5/bin/hello.elf`，可选的 `.opk` 输出到
`build-ps5/packages/hello.opk`。

## 创建插件

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

`onion_add_plugin` 会自动链接 `OnionHEN::Runtime`。`main(void)` 仍然是普通的
PS5 Payload 入口。跨宿主边界的数据必须使用公共 C 头文件定义；指针、STL
对象、异常和 C++ 类布局都不属于稳定 ABI。

## 架构

```text
插件代码
    -> Runtime + Host Services
    -> IPC Client Adapter
    -> Transport（socket、mock 或未来实现）
```

所有权边界、状态转换、ABI 约束和扩展方式见
[docs/architecture.md](docs/architecture.md)。

## 当前范围与路线图

当前版本专注于插件侧基础能力，后续宿主层按以下顺序实现：

1. OnionHEN 插件管理器和 manifest 校验
2. Host IPC broker 与 capability 强制检查
3. 插件状态、自动启动、停止/重启和崩溃恢复
4. 带 WebUI backend 的 UI Contribution API
5. 带固件适配器的 ShellUI XML backend
6. 可选的 etaHEN `.plugin` 兼容工具

## 参与贡献

提交 Pull Request 前请阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。Bug 和功能
建议请使用 GitHub Issue 表单；安全问题请按照 [SECURITY.md](SECURITY.md)
中的方式报告。

## 致谢与相关项目

- [OnionHEN](https://github.com/aydencharles/onionHEN) — 宿主项目及本仓库使用的原始 Logo
- [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk) — Prospero 工具链和目标头文件
- [etaHEN-Plugins](https://github.com/etaHEN/etaHEN-Plugins) — PS5 插件项目结构和打包约定的参考

## 许可证

本项目基于 [GNU General Public License v3.0](LICENSE) 发布。第三方组件保留
各自的许可证和声明。

> OnionHEN 是非官方自制项目，与 Sony Interactive Entertainment 无关。
> 请只在你拥有的硬件上使用，并自行承担风险。本项目不提供任何担保。
