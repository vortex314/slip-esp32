idf_component_register(
    SRCS main.cpp syslog.cpp
    INCLUDE_DIRS ../../limero/inc  
    REQUIRES  lwip SIO
    )
