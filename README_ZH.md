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

> 本仓库实现的是插件侧 SDK。OnionHEN 已具备宿主 UI registry、ShellUI XML
> adapter、协作式 daemon 插件 socket 和插件管理器。插件是包含
> `.onion_plugin` 描述符的标准 little-endian ELF，不需要 ZIP 容器或 manifest。

## 功能

- 面向 PS5 target 的 CMake `onion_add_plugin(...)`
- 写入 `.onion_plugin` section 的版本化插件描述符
- 明确的生命周期状态机和回调
- 日志、通知、配置等 capability-scoped 宿主服务
- 字符串、整数、布尔配置辅助函数
- 支持回调中安全取消订阅的线程安全事件总线
- 带 request ID 和状态响应的固定宽度 IPC frame
- 连接级 `HELLO`，绑定不可变 plugin ID 并声明 capability
- 可替换 transport，支持 socket、mock 以及未来实现
- 版本化 `onion.ui` service，支持页面、菜单、分组、标签、按钮、开关、列表、
  列表项和输入框 contribution
- 带完整校验的 little-endian UI document 与分块 IPC 注册
- 按 owner 隔离的 UI 动作轮询与严格事件解码
- 插件检查和原子部署 Python 工具
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
cmake --build build-ps5 --target hello
```

插件 ELF 输出到 `build-ps5/bin/hello.elf`。使用
`tools/deploy_plugin.py` 部署后，host 会在校验 descriptor 后安装为
`/data/OnionHEN/plugins/<plugin_id>.elf`。

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

## UI Contribution

使用 IPC client 时，应在查询可选服务前为连接建立一次会话。descriptor ID
由插件自行声明；宿主会将它绑定到当前连接并拒绝重复的活跃 ID，但不会对插件
进行密码学认证。

```c
onion_socket_transport_connect(&transport, &socket_state,
                               ONION_PLUGIN_IPC_SOCKET_PATH);
onion_client_init(&client, &transport);
onion_client_open_session(&client, &onion_plugin_descriptor);
onion_client_make_services(&client, &host_services);
```

包含 `<onion/ui.h>` 并创建 opaque `onion_ui_document`。先加入根页面，再加入
子节点；校验完成后，通过插件收到的 Host Services 注册。相同 `plugin_id` 与
`contribution_id` 的新文档会原子替换旧文档，并保持原有 handle。

```c
onion_ui_document_desc_v1 document_desc = {
    .struct_size = sizeof(document_desc),
    .abi_version = ONION_UI_ABI_VERSION,
    .plugin_id = "MYPL00001",
    .contribution_id = "settings",
    .title = "My plugin",
    .root_page_id = "main",
};
onion_ui_document *document = NULL;
onion_ui_document_create(&document_desc, &document);

onion_ui_node_desc_v1 page = {
    .struct_size = sizeof(page),
    .abi_version = ONION_UI_ABI_VERSION,
    .kind = ONION_UI_NODE_PAGE,
    .id = "main",
    .title = "Settings",
};
onion_ui_document_add_node(document, &page);
onion_ui_register(host_services, document, &handle);
```

在插件自己的事件循环中轮询动作。每次调用最多消费一个事件；队列为空时会立即
返回 `ONION_E_NOT_FOUND`。

```c
onion_ui_event_v1 event;
if (onion_client_poll_ui_event(&client, &event) == ONION_OK) {
    onion_event_publish(event_bus, ONION_EVENT_UI_ACTION,
                        &event, sizeof(event));
}
```

UI document 上限为 256 KiB、256 个节点和 8 层嵌套。插件 ID 与节点 ID 是
稳定标识，不是显示文本。插件正常停止时应主动 unregister；连接异常断开时，
宿主也会清理该会话拥有的全部 contribution。

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

当前版本已包含插件侧 UI Contribution contract、OnionHEN registry/XML adapter、
跨进程 snapshot 发布和按 owner 隔离的动作投递。剩余宿主层按以下顺序实现：

1. 日志、通知与配置等剩余 daemon Host Service handler
2. 插件状态、自动启动、停止/重启和崩溃恢复
3. 可选的 WebUI contribution backend
4. 可选的 etaHEN `.plugin` 兼容工具

当前 daemon 插件 socket 已处理 `HELLO`、`PING`、9 号事件轮询和 10–15 号 UI
命令。`onion_client_poll_ui_event()` 每次返回一个校验后的
`onion_ui_event_v1`，队列为空时返回 `ONION_E_NOT_FOUND`。日志、通知、配置等
其它 Host Service 命令在 daemon handler 完成前返回
`ONION_E_NOT_SUPPORTED`。

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
