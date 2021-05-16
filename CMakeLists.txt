# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(WIFI_SSID $ENV{WIFI_SSID})
set(WIFI_PASS $ENV{WIFI_PASS})
set(COMPONENT_SRCDIRS main)


include_directories( main  sio ../ArduinoJson/src )
add_definitions(-DWIFI_PASS=${WIFI_PASS} 
-DWIFI_SSID=${WIFI_SSID} 
-DESP32_IDF 
-DMQTT_HOST=192.168.1.1 
-DMQTT_PORT=1883 
-DHOSTNAME=triac 
-DSLIP_RX_FROM_ISR=1
)
# add_compile_options( -mtext-section-literals -ffunction-sections )

set(EXTRA_COMPONENT_DIRS /home/lieven/workspace/limero/esp32_idf components )


include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(slip-esp32)

