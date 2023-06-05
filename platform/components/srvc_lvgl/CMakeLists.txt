# LVGL source files
file(GLOB_RECURSE LVGL_SOURCES lvgl/src/*.c)

# Register the component
idf_component_register(
    SRCS
        # List of source files
        "srvc_lvgl.c"
        "${LVGL_SOURCES}"
    INCLUDE_DIRS
        # List of public include directories
        "."
        "./lvgl"
    PRIV_INCLUDE_DIRS
        # List of private include directories
        "./lvgl/src"
    REQUIRES
        # List of public required components
        "common"
    PRIV_REQUIRES
        # List of private required components
        "srvc_lcd_st7796s_demo"
        "srvc_touch_gt911"
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_CONF_INCLUDE_SIMPLE")

if (CONFIG_LV_MEM_CUSTOM)
    if (CONFIG_LV_MEM_CUSTOM_ALLOC)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_MEM_CUSTOM_ALLOC=${CONFIG_LV_MEM_CUSTOM_ALLOC}")
    endif()

    if (CONFIG_LV_MEM_CUSTOM_FREE)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_MEM_CUSTOM_FREE=${CONFIG_LV_MEM_CUSTOM_FREE}")
    endif()
endif()

if (CONFIG_LV_TICK_CUSTOM)
    if (CONFIG_LV_TICK_CUSTOM_SYS_TIME_EXPR)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_TICK_CUSTOM_SYS_TIME_EXPR=${CONFIG_LV_TICK_CUSTOM_SYS_TIME_EXPR}")
    endif()
endif()

if (CONFIG_LV_USER_DATA_FREE)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_USER_DATA_FREE=${CONFIG_LV_USER_DATA_FREE}")
endif()

if (CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_ATTRIBUTE_FAST_MEM=IRAM_ATTR")
endif()
