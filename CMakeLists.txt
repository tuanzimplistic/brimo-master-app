# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.12)

# sdkconfig is generated into "build" folder
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)

set(EXTRA_COMPONENT_DIRS "app/c_app" "app/py_app" "middleware/components" "platform/components platform/drivers")

set(MODBUS_API_PATH ${CMAKE_CURRENT_SOURCE_DIR}/middleware/api/modbus_functions)
include("${MODBUS_API_PATH}/modbus_api.cmake")


# Include main IDF cmake file and define the project.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Brimo)
