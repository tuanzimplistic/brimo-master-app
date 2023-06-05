# Register the component
idf_component_register(
    SRCS
        # List of source files
        "srvc_lcd_st7796s_demo.c"
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
        "hwa_gpio"
)
