# Register the component
idf_component_register(
    SRCS
        # List of source files
        "app_mqtt_mngr.c"
    INCLUDE_DIRS
        # List of public include directories
        "."
    PRIV_INCLUDE_DIRS
        # List of private include directories
    REQUIRES
        # List of public required components
        "common"
    PRIV_REQUIRES
        # List of private required components
        "app_ota_mngr"
        "srvc_mqtt"
        "srvc_param"
        "srvc_micropy"
        "srvc_wifi"
        "srvc_fwu_esp32"
        "json"
)
