#include <Hardware.h>
#include <Mqtt.h>
#include <MqttWifi.h>
#include <limero.h>

#include "../components/SIO/sio.cpp"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/mqtt.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/netif.h"
#include "lwip/sio.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "netif/slipif.h"
#include "syslog.h"

Log logger(1024);
Syslog syslog;
Thread mqttThread("mqtt");
MqttWifi mqtt(mqttThread);

struct netif sl_netif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint32_t interface = 0;

static void status_callback(struct netif *state_netif) {
  if (netif_is_up(state_netif)) {
#if LWIP_IPV4
    INFO("status_callback==UP, local interface IP is %s",
         ip4addr_ntoa(netif_ip4_addr(state_netif)));
#else
    INFO("status_callback==UP\n");
#endif
    //   mqtt.wifiConnected.on(true);

  } else {
    INFO("status_callback==DOWN\n");
  }
}

static void link_callback(struct netif *state_netif) {
  if (netif_is_link_up(state_netif)) {
    INFO("link_callback==UP");
  } else {
    INFO("link_callback==DOWN");
  }
}

ValueSource<std::string> systemBuild("NOT SET");
ValueSource<std::string> systemHostname("NOT SET");
ValueSource<bool> systemAlive = true;
LambdaSource<uint32_t> systemHeap([]() { return Sys::getFreeHeap(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
Poller poller(mqttThread);

extern "C" void app_main(void) {
  INFO(" Starting build : %s ", __DATE__);
  tcpip_init([](void *) { INFO(" TCP ready "); }, NULL);
  udp_init();
  INFO(" setting SLIP config ");
  IP4_ADDR(&ipaddr, 192, 168, 1, 2);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 192, 168, 1, 1);
  sl_netif.num = 0;
  struct netif *nif = netif_add(&sl_netif, &ipaddr, &netmask, &gw, &interface,
                                slipif_init, ip_input);

  if (nif != 0) {
    netif_set_status_callback(&sl_netif, status_callback);
    netif_set_default(&sl_netif);
    netif_set_link_up(&sl_netif);
    netif_set_up(&sl_netif);
    //   netif_set_link_callback(&sl_netif, link_callback);
    vTaskDelay(10);
    syslog.init("192.168.1.1", 514);
    //    mqtt.init();
  }
  //  mqtt.wifiConnected.on(true);
  poller.connected = true;
  //-----------------------------------------------------------------  SYS props
  poller >> systemUptime >> mqtt.toTopic<uint64_t>("system/upTime");
  poller >> systemHeap >> mqtt.toTopic<uint32_t>("system/heap");
  poller >> systemHostname >> mqtt.toTopic<std::string>("system/hostname");
  poller >> systemBuild >> mqtt.toTopic<std::string>("system/build");
  poller >> systemAlive >> mqtt.toTopic<bool>("system/alive");
  mqttThread.start();
  vTaskDelay(10);
  //  mqtt.wifiConnected.on(true);
  while (true) {
    std::string msg = "UDP Log ";
    //    syslog.log(msg.c_str(), msg.length());
    vTaskDelay(100);
  }
}