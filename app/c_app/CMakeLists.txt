idf_component_register(
    SRCS
        "app_main.c"
    INCLUDE_DIRS
        # List of public include directories
        "."
    PRIV_INCLUDE_DIRS
        "../../middleware/components/common"
        "../../middleware/components/app_gui_mngr"
        "../../middleware/components/srvc_micropy"
        "../../middleware/api/modbus_functions"
        "../../middleware/api/modbus_functions/request"
        "../../platform/components/srvc_param"
        "../../platform/components/app_mqtt_mngr"
        "../../platform/components/app_ota_mngr"
        "../../platform/components/app_wifi_mngr"
        "../../platform/components/srvc_recovery"
        "../../platform/components/freemodbus/FreeModbus/modbus/include"
        "../../platform/components/freemodbus/FreeModbus/port/zpl_esp32"
    REQUIRES
        # List of public required components
    PRIV_REQUIRES
        "common"
        "app_ota_mngr"
        "app_mqtt_mngr"
        "app_wifi_mngr"
        "srvc_recovery"
)