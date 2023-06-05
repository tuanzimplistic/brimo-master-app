# Set location of base MicroPython directory
set(MICROPY_DIR ${CMAKE_CURRENT_LIST_DIR}/micropy)

# Set user C-binding modules directory
set(MICROPY_CMODULE_DIR ${CMAKE_CURRENT_LIST_DIR}/modules)

# Set the location of ESP32 port's directory
set(MICROPY_PORT_DIR ${MICROPY_DIR}/esp32)

# Set the board directory
set(MICROPY_BOARD_DIR ${MICROPY_PORT_DIR}/boards)

# Build mpy-cross which will be used to build MicroPython
execute_process(COMMAND make -C ${MICROPY_DIR}/mpy-cross)

# Include board config
include(${MICROPY_BOARD_DIR}/mpconfigboard.cmake)

# Include MicroPython core source components
include(${MICROPY_DIR}/py/py.cmake)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    include(${MICROPY_DIR}/py/usermod.cmake)
    include(${MICROPY_DIR}/extmod/extmod.cmake)
endif()

set(MICROPY_QSTRDEFS_PORT ${MICROPY_PORT_DIR}/qstrdefsport.h)

set(MICROPY_SOURCE_SHARED
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/netutils/netutils.c
    ${MICROPY_DIR}/shared/timeutils/timeutils.c
    ${MICROPY_DIR}/shared/runtime/interrupt_char.c
    ${MICROPY_DIR}/shared/runtime/stdout_helpers.c
    ${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c
    ${MICROPY_DIR}/shared/runtime/pyexec.c
)

set(MICROPY_SOURCE_LIB
    ${MICROPY_DIR}/lib/littlefs/lfs2.c
    ${MICROPY_DIR}/lib/littlefs/lfs2_util.c
    ${MICROPY_DIR}/lib/mbedtls_errors/mp_mbedtls_errors.c
    ${MICROPY_DIR}/lib/oofatfs/ff.c
    ${MICROPY_DIR}/lib/oofatfs/ffunicode.c
)

# Port of MicroPython for ESP32
set(MICROPY_SOURCE_PORT
    ${MICROPY_PORT_DIR}/fatfs_port.c
    ${MICROPY_PORT_DIR}/gccollect.c
    ${MICROPY_PORT_DIR}/mphalport.c
    ${MICROPY_PORT_DIR}/help.c
    ${MICROPY_PORT_DIR}/modutime.c
    ${MICROPY_PORT_DIR}/moduos.c
    ${MICROPY_PORT_DIR}/machine_pin.c
    ${MICROPY_PORT_DIR}/modmachine.c
    ${MICROPY_PORT_DIR}/modsocket.c
    ${MICROPY_PORT_DIR}/modesp.c
    ${MICROPY_PORT_DIR}/esp32_nvs.c
    ${MICROPY_PORT_DIR}/esp32_partition.c
    ${MICROPY_PORT_DIR}/modesp32.c
    ${MICROPY_PORT_DIR}/mpthreadport.c
    ${MICROPY_PORT_DIR}/mp_port.c
)

# C-binding modules for Itor3 project
set(MICROPY_SOURCE_ITOR3_MOD
    ${MICROPY_CMODULE_DIR}/cmp_queue_binding.c
    ${MICROPY_CMODULE_DIR}/gui_binding.c
    ${MICROPY_CMODULE_DIR}/cam_binding.c
    ${MICROPY_CMODULE_DIR}/param_binding.c
    ${MICROPY_CMODULE_DIR}/ota_binding.c
    ${MICROPY_CMODULE_DIR}/ws_notify_binding.c
    ${MICROPY_CMODULE_DIR}/recovery_binding.c
)

set(MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_USERMOD}
    ${MICROPY_SOURCE_SHARED}
    ${MICROPY_SOURCE_LIB}
    ${MICROPY_SOURCE_PORT}
    ${MICROPY_SOURCE_ITOR3_MOD}
)

set(IDF_COMPONENTS
    app_update
    bootloader_support
    driver
    esp32
    esp_common
    esp_eth
    esp_event
    esp_hw_support
    esp_netif
    esp_pm
    esp_ringbuf
    esp_rom
    esp_system
    esp_timer
    esp_wifi
    freertos
    hal
    heap
    log
    lwip
    mbedtls
    mdns
    newlib
    nvs_flash
    soc
    spi_flash
    tcpip_adapter
    vfs
    xtensa
)

# Register the component.
idf_component_register(
    SRCS
        # List of source files
        srvc_micropy.c
        ${MICROPY_CMODULE_DIR}/cmp_queue.c
        ${MICROPY_CMODULE_DIR}/gui.c
        ${MICROPY_CMODULE_DIR}/decode.c
        ${MICROPY_CMODULE_DIR}/identify.c
        ${MICROPY_CMODULE_DIR}/quirc.c
        ${MICROPY_CMODULE_DIR}/version_db.c
        ${MICROPY_CMODULE_DIR}/cam.c
        ${MICROPY_CMODULE_DIR}/param.c
        ${MICROPY_CMODULE_DIR}/ota.c
        ${MICROPY_CMODULE_DIR}/ws_notify.c
        ${MICROPY_CMODULE_DIR}/recovery.c
        ${MICROPY_SOURCE_ITOR3_MOD}
        ${MICROPY_SOURCE_PY}
        ${MICROPY_SOURCE_EXTMOD}
        ${MICROPY_SOURCE_SHARED}
        ${MICROPY_SOURCE_LIB}
        ${MICROPY_SOURCE_PORT}
    INCLUDE_DIRS
        # List of public include directories
        "."
    PRIV_INCLUDE_DIRS
        # List of private include directories
        ${MICROPY_INC_CORE}
        ${MICROPY_INC_USERMOD}
        ${MICROPY_PORT_DIR}
        ${MICROPY_BOARD_DIR}
        ${CMAKE_BINARY_DIR}
        ${MICROPY_CMODULE_DIR}
    REQUIRES
        # List of public required components
        ${IDF_COMPONENTS}
        common
    PRIV_REQUIRES
        # List of private required components
        app_gui_mngr
        app_ota_mngr
        srvc_cam
        srvc_ws_server
        srvc_param
        srvc_recovery
)

# Set the MicroPython target as the current (main) IDF component target.
set(MICROPY_TARGET ${COMPONENT_TARGET})

# Define mpy-cross flags, for use with frozen code.
set(MICROPY_CROSS_FLAGS -march=xtensawin)

# Set compile options for this port.
target_compile_definitions(${MICROPY_TARGET} PUBLIC
    ${MICROPY_DEF_CORE}
    MICROPY_ESP_IDF_4=1
    MICROPY_VFS_FAT=1
    MICROPY_VFS_LFS2=1
    FFCONF_H=\"${MICROPY_OOFATFS_DIR}/ffconf.h\"
    LFS2_NO_DEBUG LFS2_NO_WARN LFS2_NO_ERROR LFS2_NO_ASSERT
)

# Disable some warnings to keep the build output clean.
target_compile_options(${MICROPY_TARGET} PUBLIC
    -Wno-clobbered
    -Wno-deprecated-declarations
    -Wno-missing-field-initializers
)

# Add additional extmod and usermod components.
target_link_libraries(${MICROPY_TARGET} usermod)

# Collect all of the include directories and compile definitions for the IDF components.
foreach(comp ${IDF_COMPONENTS})
    micropy_gather_target_properties(__idf_${comp})
endforeach()

if(IDF_VERSION_MINOR GREATER_EQUAL 2)
    # These paths cannot currently be found by the IDF_COMPONENTS search loop above,
    # so add them explicitly.
    list(APPEND MICROPY_CPP_INC_EXTRA ${IDF_PATH}/components/soc/soc/${IDF_TARGET}/include)
    list(APPEND MICROPY_CPP_INC_EXTRA ${IDF_PATH}/components/soc/soc/include)
endif()

# Include the main MicroPython cmake rules.
include(${MICROPY_DIR}/py/mkrules.cmake)
