#include "lwip/sio.h"

#include "Hardware.h"
#include "driver/uart.h"
#include "limero.h"
#include "lwip/opt.h"
#include "netif/slipif.h"

extern struct netif sl_netif;
extern "C" void slipif_rxbyte_input(struct netif *netif, u8_t c);

bool uart_poll;

static uint64_t lastByteRead;
static uint64_t firstByteSend;
static uint32_t count = 0;

#define TXD_PIN 21
#define RXD_PIN 19
UART &uart = UART::create(UART_NUM_1, TXD_PIN, RXD_PIN);

void IRAM_ATTR onUartRxd(void *) {
  lastByteRead = Sys::millis();
  firstByteSend = 0;
  uint32_t cnt=0;
  while (uart.hasData()) {
    cnt++;
    uint8_t c = uart.read();
    slipif_rxbyte_input(&sl_netif, c);
    //  sl_netif.input(c);
    //    slipif_received_byte(&sl_netif, c);
  }
  if ( cnt > count ) count=cnt;
}

/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
extern "C" sio_fd_t IRAM_ATTR sio_open(u8_t devnum) {
  uart.mode("8N1");
  uart.setClock(1000000);
  uart.init();
  uart.onRxd(onUartRxd, 0);
  return (sio_fd_t)1;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
extern "C" void IRAM_ATTR sio_send(u8_t c, sio_fd_t fd) {
  uart.write(c);
  if (firstByteSend == 0) {
    firstByteSend = Sys::millis();
    INFO(" delta : %lu max chars :%d ", firstByteSend - lastByteRead,count);
  }
}

/**
 * Receives a single character from the serial device.
 *
 * @param fd serial device handle
 *
 * @note This function will block until a character is received.
 */
extern "C" u8_t IRAM_ATTR sio_recv(sio_fd_t fd) {
  while (true) {
    if (uart.hasData()) return uart.read();
    vTaskDelay(1);
  }
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by
 * sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
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

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
extern "C" u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len) {
  int count = 0;
  while (uart.hasData() && count < len) {
    data[count] = uart.read();
    count++;
  }
  return count;
}

/**
 * Writes to the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data to send
 * @param len length (in bytes) of data to send
 * @return number of bytes actually sent
 *
 * @note This function will block until all data can be sent.
 */
extern "C" u32_t IRAM_ATTR sio_write(sio_fd_t fd, u8_t *data, u32_t len) {
  uart.write(data, len);
  return len;
}

/**
 * Aborts a blocking sio_read() call.
 *
 * @param fd serial device handle
 */
extern "C" void IRAM_ATTR sio_read_abort(sio_fd_t fd) { uart_poll = false; }
