#include <Hardware.h>
#include <Log.h>
#include <Sio.h>

#include "lwip/apps/mqtt.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/netif.h"
#include "lwip/sio.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "netif/slipif.h"

extern "C" void slipif_rxbyte_input(struct netif *netif, u8_t c);

#define TXD_PIN 21
#define RXD_PIN 19

#define UART_USED UART_NUM_1

#if UART_USED ==  UART_NUM_0
#define BAUDRATE 921600
#endif
#if UART_USED ==  UART_NUM_1
#define BAUDRATE 1000000
#endif

struct netif sl_netif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint32_t interface = 0;

//______________________________________________________________________________________________

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

bool SlipInit() {
  tcpip_init([](void *) { INFO(" TCP ready "); }, NULL);
  udp_init();
 /* Sio::createInstance(1600, 0x7E, TXD_PIN, RXD_PIN, BAUDRATE,
                      [](uint8_t *buffer, uint32_t length) {
                        ip_input(ppp, buffer, length);
                      });*/
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
    return true;
  }
  return false;
}
