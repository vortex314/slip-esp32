# Purpose
- Use USB connected microcontrollers as slave devices from Raspberry Pi 
- run SLIP via USB 
- run MQTT client on microcontroller
- run telnet CLI on microcontroller
# build 
CONFIG_LWIP_NETIF_STATUS_CALLBACK=y

# Issues
```/home/lieven/esp/esp-idf/components/lwip/lwip/src/netif/slipif.c:437: undefined reference to `sio_tryread'```
- related to ???