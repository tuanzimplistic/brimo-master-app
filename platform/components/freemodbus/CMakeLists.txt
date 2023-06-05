
# Register the component
idf_component_register(
    SRCS
        # List of source files
        "FreeModbus/modbus/mb_m.c"
        "FreeModbus/port/zpl_esp32/port.c"
        "FreeModbus/port/zpl_esp32/portevent_m.c"
        "FreeModbus/port/zpl_esp32/portother_m.c"
        "FreeModbus/port/zpl_esp32/portserial_m.c"
        "FreeModbus/port/zpl_esp32/porttimer_m.c"
        "FreeModbus/modbus/zpl/mbcrc.c"
        "FreeModbus/modbus/zpl/mbzpl_m.c"
        ${MODBUS_SOURCE_API}
    INCLUDE_DIRS
        # List of public include directories
        "FreeModbus/port/zpl_esp32"
        "FreeModbus/modbus/include"
    PRIV_INCLUDE_DIRS
        # List of private include directories
        "FreeModbus/modbus"
        ${MODBUS_HEADER_API}
        "FreeModbus/modbus/zpl"
    REQUIRES
        # List of required public components
    PRIV_REQUIRES
        # List of required private components
        "srvc_micropy"
        "srvc_rt_log"
)

