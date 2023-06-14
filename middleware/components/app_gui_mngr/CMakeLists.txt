# Register the component
idf_component_register(
    SRCS
        # List of source files
        "app_gui_mngr.c"

        # List of fonts
        "fonts/arial_96.c"
        "fonts/arial_bold_18.c"
        "fonts/wifi_symbol.c"

        # List of screens
        "screens/screen_common.c"
        "screens/splash.c"
        "screens/cam_screen.c"
        "screens/x_making.c"
        "screens/wifi_setting.c"
        "screens/virtual_keyboard.c"
        "screens/menu.c"
        "screens/developer.c"
        "screens/resources/img_invisible.c"
        "screens/resources/img_visible.c"
        "screens/resources/img_play.c"
        "screens/resources/img_pause.c"
        "screens/resources/img_plus.c"
        "screens/resources/img_minus.c"
        "screens/resources/img_back.c"
        "screens/resources/img_wifi.c"
        "screens/resources/img_debug.c"
        "screens/resources/img_about.c"

        # List of controls
        "controls/control_common.c"
        "controls/notify_msgbox.c"
        "controls/query_msgbox.c"
        "controls/progress_msgbox.c"
        "controls/resources/img_info.c"
        "controls/resources/img_warning.c"
        "controls/resources/img_error.c"
        "controls/resources/img_system.c"
    INCLUDE_DIRS
        # List of public include directories
        "."
    PRIV_INCLUDE_DIRS
        # List of private include directories
        "screens"
        "controls"
    REQUIRES
        # List of public required components
        "common"
    PRIV_REQUIRES
        # List of private required components
        "app_wifi_mngr"
        "srvc_lvgl"
        "srvc_wifi"
        "srvc_cam"
        "srvc_param"
        "srvc_fwu_esp32"
        "srvc_recovery"
)
