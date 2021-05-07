#include <Hardware.h>
#include <limero.h>

#include "../components/SIO/sio.cpp"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/netif.h"
#include "lwip/sio.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "netif/slipif.h"
#include "syslog.h"

Log logger(1024);
Syslog syslog;

struct netif sl_netif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint32_t interface = 0;

static void status_callback(struct netif* state_netif) {
  if (netif_is_up(state_netif)) {
#if LWIP_IPV4
    INFO("status_callback==UP, local interface IP is %s",
         ip4addr_ntoa(netif_ip4_addr(state_netif)));
#else
    INFO("status_callback==UP\n");
#endif
  } else {
    INFO("status_callback==DOWN\n");
  }
}

static void link_callback(struct netif* state_netif) {
  if (netif_is_link_up(state_netif)) {
    INFO("link_callback==UP");
  } else {
    INFO("link_callback==DOWN");
  }
}

extern "C" void app_main(void) {
  INFO(" Starting build : %s ", __DATE__);
  INFO(" setting SLIP config ");
  IP4_ADDR(&ipaddr, 192, 168, 1, 2);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 192, 168, 1, 1);
  sl_netif.num = 0;
  struct netif* nif = netif_add(&sl_netif, &ipaddr, &netmask, &gw, &interface,
                                slipif_init, ip_input);

  if (nif != 0) {
    netif_set_default(&sl_netif);
    netif_set_status_callback(&sl_netif, status_callback);
    //   netif_set_link_callback(&sl_netif, link_callback);

    netif_set_up(&sl_netif);
    tcpip_init(NULL, NULL);
    if (syslog.init("192.168.1.1", 514)) {
      std::string msg = "UDP Log ";
      syslog.log(msg.c_str(), msg.length());
    }
  }
  while (true) {
    std::string msg = "UDP Log ";
    syslog.log(msg.c_str(), msg.length());
    vTaskDelay(100);
  }
}