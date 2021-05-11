idf_component_register(
    SRCS main.cpp Syslog.cpp MqttClient.cpp ${IDF_PATH}/components/lwip/lwip/src/apps/mqtt/mqtt.c Slip.cpp Ppp.cpp Sio.cpp
    INCLUDE_DIRS ../../limero/inc  ../../limero/esp32_idf ${IDF_PATH}/components/lwip/lwip/src/include/lwip/apps/
    REQUIRES  lwip  mqtt esp_https_ota app_update
    )
