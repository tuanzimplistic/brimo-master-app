# Register the component
idf_component_register(
    SRCS
        # List of source files
        "srvc_fwu_slave.c"
    INCLUDE_DIRS
        # List of public include directories
        "."
        ${MODBUS_HEADER_API}
    PRIV_INCLUDE_DIRS
        # List of private include directories
    REQUIRES
        # List of public required components
        "common"
    PRIV_REQUIRES
        # List of private required components
        "srvc_master_commander"
        "freemodbus"
)
