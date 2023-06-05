# Register the component
idf_component_register(
    SRCS
        # List of source files
        "srvc_touch_gt911.c"
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
        "srvc_io_tca9534"
        "hwa_i2c_master"
        "hwa_gpio"
)
