#include <Hardware.h>
#include <Log.h>

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

#define FTDI

struct netif sl_netif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint32_t interface = 0;
bool uart_poll = false;

//______________________________________________________________________________________________
#define TXD_PIN 21
#define RXD_PIN 19
#define UART_NUM_1 1

#ifdef CP2102
UART &uart =
    UART::create(UART_NUM_0, TXD_PIN, RXD_PIN);  // pins not used,onboard USB
#define BAUDRATE 921600
#endif
#ifdef FTDI
UART &uart = UART::create(UART_NUM_1, TXD_PIN, RXD_PIN);
#define BAUDRATE 1000000
#endif

void IRAM_ATTR onUartRxd(void *) {
  while (uart.hasData()) {
    uint8_t c = uart.read();
    slipif_rxbyte_input(&sl_netif, c);
  }
}

extern "C" sio_fd_t IRAM_ATTR sio_open(u8_t devnum) {
  uart.mode("8N1");
  uart.setClock(BAUDRATE);
  uart.init();
  uart.onRxd(onUartRxd, 0);
  return (sio_fd_t)1;
}

extern "C" void IRAM_ATTR sio_send(u8_t c, sio_fd_t fd) { uart.write(c); }

extern "C" u32_t IRAM_ATTR sio_read(sio_fd_t fd, u8_t *data, u32_t len) {
  uart_poll = true;
  int count = 0;
  while (uart_poll) {
    while (uart.hasData() && count < len) {
      data[count] = uart.read();
      count++;
    }
    if (count) return count;
  }
  return 0;
}
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
