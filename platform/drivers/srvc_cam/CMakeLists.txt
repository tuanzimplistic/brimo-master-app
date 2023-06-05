# Register the component
idf_component_register(
    SRCS
        # List of source files
        "srvc_cam.c"
        "driver/esp_camera.c"
        "driver/cam_hal.c"
        "driver/sccb.c"
        "driver/sensor.c"
        "sensor/ov2640.c"
        "conversions/to_jpg.cpp"
        "conversions/jpge.cpp"
        "conversions/esp_jpg_decode.c"
        "conversions/yuv.c"
        "conversions/to_bmp.c"
        "target/xclk.c"
        "target/esp32/ll_cam.c"
    INCLUDE_DIRS
        # List of public include directories
        "."
        "conversions/include"
        "driver/include"
    PRIV_INCLUDE_DIRS
        # List of private include directories
        "driver/private_include"
        "conversions/private_include"
        "sensor/private_include"
        "target/private_include"
    REQUIRES
        # List of public required components
        "common"
    PRIV_REQUIRES
        # List of private required components
        "srvc_io_tca9534"
        "hwa_i2c_master"
        "hwa_gpio"
        "nvs_flash"
)
