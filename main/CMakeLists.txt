idf_component_register(
    SRCS main.cpp syslog.cpp
    INCLUDE_DIRS ../../limero/inc  ../../limero/esp32_idf 
    REQUIRES  lwip  mqtt esp_https_ota app_update
    )
