include_guard(GLOBAL)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Add one standalone ELF plugin.  The function intentionally keeps all PS5
# flags target-local so the SDK can be used from a larger CMake project.
function(onion_add_plugin)
    set(options)
    set(oneValueArgs NAME TITLE_ID VERSION)
    set(multiValueArgs SOURCES INCLUDE_DIRS LIBRARIES COMPILE_OPTIONS)
    cmake_parse_arguments(OP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(required NAME TITLE_ID VERSION)
        if(NOT OP_${required})
            message(FATAL_ERROR "onion_add_plugin: ${required} is required")
        endif()
    endforeach()
    if(NOT OP_SOURCES)
        message(FATAL_ERROR "onion_add_plugin(${OP_NAME}): SOURCES is required")
    endif()

    if(ONION_SDK_ROOT)
        set(_sdk_root "${ONION_SDK_ROOT}")
        set(_sdk_tools "${_sdk_root}/bin")
    else()
        set(_sdk_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
        set(_sdk_tools "${_sdk_root}/tools")
    endif()
    if(NOT PS5_PAYLOAD_SDK)
        if(DEFINED ENV{PS5_PAYLOAD_SDK})
            set(PS5_PAYLOAD_SDK "$ENV{PS5_PAYLOAD_SDK}")
        else()
            message(FATAL_ERROR
                "onion_add_plugin(${OP_NAME}): set PS5_PAYLOAD_SDK to a ps5-payload-sdk checkout")
        endif()
    endif()
    if(NOT EXISTS "${PS5_PAYLOAD_SDK}/include")
        message(FATAL_ERROR "PS5_PAYLOAD_SDK has no include directory: ${PS5_PAYLOAD_SDK}")
    endif()

    add_executable(${OP_NAME} ${OP_SOURCES})
    set_target_properties(${OP_NAME} PROPERTIES
        OUTPUT_NAME "${OP_NAME}.elf"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    target_compile_features(${OP_NAME} PRIVATE c_std_11)
    target_compile_options(${OP_NAME} PRIVATE
        --target=x86_64-sie-ps5
        -DPPR -DPS5
        -march=znver2
        -fPIC -fPIE
        -ffunction-sections -fdata-sections
        -Wall -Wextra -Werror
        ${OP_COMPILE_OPTIONS})
    target_include_directories(${OP_NAME} PRIVATE
        "${PS5_PAYLOAD_SDK}"
        "${PS5_PAYLOAD_SDK}/include"
        "${OP_INCLUDE_DIRS}")
    if(EXISTS "${PS5_PAYLOAD_SDK}/target/lib")
        target_link_directories(${OP_NAME} PRIVATE "${PS5_PAYLOAD_SDK}/target/lib")
    endif()
    target_link_directories(${OP_NAME} PRIVATE "${_sdk_root}/lib")
    # Every plugin gets the SDK runtime. Optional PS5/system libraries remain
    # explicit at the call site, keeping dependency ownership visible.
    target_link_libraries(${OP_NAME} PRIVATE OnionHEN::Runtime ${OP_LIBRARIES})

    find_program(_objcopy NAMES llvm-objcopy prospero-objcopy)
    if(_objcopy)
        add_custom_command(TARGET ${OP_NAME} POST_BUILD
            COMMAND "${_objcopy}" --strip-unneeded "$<TARGET_FILE:${OP_NAME}>"
            COMMENT "Stripping ${OP_NAME}.elf")
    endif()

    set(_package_dir "${CMAKE_BINARY_DIR}/packages")
    set(_package "${_package_dir}/${OP_NAME}.opk")
    add_custom_command(OUTPUT "${_package}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_package_dir}"
        COMMAND "${Python3_EXECUTABLE}" "${_sdk_tools}/pack_plugin.py"
            "$<TARGET_FILE:${OP_NAME}>"
            --id "${OP_TITLE_ID}"
            --version "${OP_VERSION}"
            --output "${_package}"
        DEPENDS ${OP_NAME} "${_sdk_tools}/pack_plugin.py"
        COMMENT "Packaging ${OP_NAME}.opk"
        VERBATIM)
    add_custom_target(${OP_NAME}_package DEPENDS "${_package}")
    add_dependencies(${OP_NAME}_package ${OP_NAME})

    set(${OP_NAME}_ELF "$<TARGET_FILE:${OP_NAME}>" PARENT_SCOPE)
    set(${OP_NAME}_PACKAGE "${_package}" PARENT_SCOPE)
endfunction()
