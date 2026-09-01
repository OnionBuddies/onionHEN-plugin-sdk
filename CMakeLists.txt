cmake_minimum_required(VERSION 3.20)

project(OnionHENPluginSDK VERSION 0.1.0 LANGUAGES C)

option(ONION_SDK_BUILD_SAMPLES "Build the PS5 plugin samples" OFF)
option(ONION_SDK_BUILD_TESTS "Build host-side SDK tests" OFF)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Threads REQUIRED)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(onion_sdk_headers INTERFACE)
add_library(OnionHEN::SDK ALIAS onion_sdk_headers)
set_target_properties(onion_sdk_headers PROPERTIES EXPORT_NAME SDK)
target_include_directories(onion_sdk_headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

include(OnionPlugin)
add_subdirectory(runtime)

if(ONION_SDK_BUILD_TESTS)
    enable_testing()
    add_executable(sdk_runtime_test tests/test_runtime.c)
    target_link_libraries(sdk_runtime_test PRIVATE OnionHEN::Runtime Threads::Threads)
    target_compile_features(sdk_runtime_test PRIVATE c_std_11)
    target_compile_options(sdk_runtime_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME sdk_runtime COMMAND sdk_runtime_test)
    add_test(NAME sdk_python_tools
        COMMAND ${Python3_EXECUTABLE} -c
            "import importlib.util; s=importlib.util.spec_from_file_location('t','${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tools.py'); m=importlib.util.module_from_spec(s); s.loader.exec_module(m); m.test_pack_and_inspect()")
endif()

if(ONION_SDK_BUILD_SAMPLES)
    add_subdirectory(samples/hello)
    add_subdirectory(samples/daemon)
endif()

include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES cmake/OnionPlugin.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OnionHENPluginSDK)
install(PROGRAMS
    tools/pack_plugin.py
    tools/inspect_plugin.py
    tools/deploy_plugin.py
    DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/OnionHENPluginSDKConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/OnionHENPluginSDKConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OnionHENPluginSDK)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/OnionHENPluginSDKConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion)
install(EXPORT OnionHENPluginSDKTargets
    FILE OnionHENPluginSDKTargets.cmake
    NAMESPACE OnionHEN::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OnionHENPluginSDK)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OnionHENPluginSDKConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OnionHENPluginSDKConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OnionHENPluginSDK)
install(TARGETS onion_sdk_headers onion_sdk_runtime EXPORT OnionHENPluginSDKTargets)
