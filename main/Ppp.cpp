#include "netif/ppp/ppp.h"

#include <Hardware.h>
#include <Log.h>

#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/netif.h"
#include "lwip/sio.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"
#include "netif/ppp/pppos.h"

//______________________________________________________________________________________________
#define TXD_PIN 21
#define RXD_PIN 19
#define UART_NUM_1 1

#define FTDI

#ifdef CP2102
UART &uart =
    UART::create(UART_NUM_0, TXD_PIN, RXD_PIN);  // pins not used,onboard USB
#define BAUDRATE 921600
#endif
#ifdef FTDI
UART &uart = UART::create(UART_NUM_1, TXD_PIN, RXD_PIN);
#define BAUDRATE 1000000
#endif

bool uart_poll = false;
static sio_fd_t ppp_sio;
static ppp_pcb *ppp;
static struct netif pppos_netif;
uint8_t data[100];

void IRAM_ATTR onUartRxd(void *) {
  int count = 0;
  while (uart.hasData() && count < sizeof(data)) {
    data[count++] = uart.read();
  }
  pppos_input_tcpip(ppp, data, count);
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
    vTaskDelay(1);
  }
  return 0;
}

extern "C" u32_t IRAM_ATTR sio_write(sio_fd_t fd, u8_t *data, u32_t len) {
  uart.write(data, len);
  return len;
}
//______________________________________________________________________________________________

static void pppos_rx_thread(void *arg) {
  u32_t len;
  u8_t buffer[128];
  LWIP_UNUSED_ARG(arg);

  /* Please read the "PPPoS input path" chapter in the PPP documentation. */
  while (1) {
    len = sio_read(ppp_sio, buffer, sizeof(buffer));
    if (len > 0) {
      /* Pass received raw characters from PPPoS to be decoded through lwIP
       * TCPIP thread using the TCPIP API. This is thread safe in all cases
       * but you should avoid passing data byte after byte. */
      pppos_input_tcpip(ppp, buffer, len);
    }
  }
}

static void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx) {
  struct netif *pppif = ppp_netif(pcb);
  LWIP_UNUSED_ARG(ctx);

  switch (err_code) {
    case PPPERR_NONE: /* No error. */
    {
#if LWIP_DNS
      const ip_addr_t *ns;
#endif /* LWIP_DNS */
      WARN("ppp_link_status_cb: PPPERR_NONE");
#if LWIP_IPV4
      INFO("our_ip4addr = %s", ip4addr_ntoa(netif_ip4_addr(pppif)));
      INFO("his_ipaddr  = %s", ip4addr_ntoa(netif_ip4_gw(pppif)));
      INFO("netmask     = %s", ip4addr_ntoa(netif_ip4_netmask(pppif)));
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
      fprintf(stderr, "   our_ip6addr = %s",
              ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* LWIP_IPV6 */

#if LWIP_DNS
      ns = dns_getserver(0);
      fprintf(stderr, "   dns1        = %s", ipaddr_ntoa(ns));
      ns = dns_getserver(1);
      fprintf(stderr, "   dns2        = %s", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#if PPP_IPV6_SUPPORT
      fprintf(stderr, "   our6_ipaddr = %s",
              ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
      break;
    }

    case PPPERR_PARAM: /* Invalid parameter. */ {
      INFO("ppp_link_status_cb: PPPERR_PARAM");
      break;
    }
    case PPPERR_OPEN: /* Unable to open PPP session. */ {
      INFO("ppp_link_status_cb: PPPERR_OPEN");
      break;
    }

    case PPPERR_DEVICE: /* Invalid I/O device for PPP. */ {
      INFO("ppp_link_status_cb: PPPERR_DEVICE");
      break;
    }

    case PPPERR_ALLOC: /* Unable to allocate resources. */ {
      INFO("ppp_link_status_cb: PPPERR_ALLOC");
      break;
    }

    case PPPERR_USER: /* User interrupt. */ {
      INFO("ppp_link_status_cb: PPPERR_USER");
      break;
    }

    case PPPERR_CONNECT: /* Connection lost. */ {
      INFO("ppp_link_status_cb: PPPERR_CONNECT");
      break;
    }

    case PPPERR_AUTHFAIL: /* Failed authentication challenge. */ {
      INFO("ppp_link_status_cb: PPPERR_AUTHFAIL");
      break;
    }

    case PPPERR_PROTOCOL: /* Failed to meet protocol. */ {
      INFO("ppp_link_status_cb: PPPERR_PROTOCOL");
      break;
    }

    case PPPERR_PEERDEAD: /* Connection timeout. */ {
      INFO("ppp_link_status_cb: PPPERR_PEERDEAD");
      break;
    }

    case PPPERR_IDLETIMEOUT: /* Idle Timeout. */ {
      INFO("ppp_link_status_cb: PPPERR_IDLETIMEOUT");
      break;
    }

    case PPPERR_CONNECTTIME: /* PPPERR_CONNECTTIME. */ {
      INFO("ppp_link_status_cb: PPPERR_CONNECTTIME");
      break;
    }

    case PPPERR_LOOPBACK: /* Connection timeout. */ {
      INFO("ppp_link_status_cb: PPPERR_LOOPBACK");
      break;
    }

    default: {
      INFO("ppp_link_status_cb: unknown errCode %d", err_code);
      break;
    }
  }
}

static u32_t ppp_output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) {
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(ctx);
  return sio_write(ppp_sio, data, len);
}

static void netif_status_callback(struct netif *nif) {
  INFO("PPPNETIF: %c%c%d is %s", nif->name[0], nif->name[1], nif->num,
       netif_is_up(nif) ? "UP" : "DOWN");
  INFO("IPV4: Host at %s", ip4addr_ntoa(netif_ip4_addr(nif)));
  INFO("mask %s", ip4addr_ntoa(netif_ip4_netmask(nif)));
  INFO("gateway %s", ip4addr_ntoa(netif_ip4_gw(nif)));
  //  INFO("FQDN: %s", netif_get_hostname(nif));
}

bool PppInit(void) {
  tcpip_init([](void *) { INFO(" TCP ready "); }, NULL);
  udp_init();
  ppp_sio = sio_open(0);
  if (!ppp_sio) {
    WARN("PPPOS example: Error opening device");
    return false;
  }

  ppp = pppos_create(&pppos_netif, ppp_output_cb, ppp_link_status_cb, NULL);
  if (!ppp) {
    WARN("PPPOS example: Could not create PPP control interface");
    return false;
  }

  ppp_connect(ppp, 0);

  netif_set_status_callback(&pppos_netif, netif_status_callback);

 /* sys_thread_new("pppos_rx_thread", pppos_rx_thread, NULL,
                 DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);*/
  return true;
}