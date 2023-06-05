# Register the component
idf_component_register(
    SRCS
        # List of source files
        "app_ota_mngr.c"
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
        "app_mqtt_mngr"
        "app_gui_mngr"
        "srvc_fwu_esp32"
        "srvc_fwu_slave"
        "esp_http_client"
        "app_update"
    EMBED_TXTFILES
        # List of files embedded into the firmware
        "server_certs/ca_cert_aws_s3.pem"
)
