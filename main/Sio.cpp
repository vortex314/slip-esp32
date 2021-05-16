#include "Sio.h"

extern struct netif sl_netif;
extern struct netif ppp_netif;
extern "C" void slipif_rxbyte_input(struct netif *netif, u8_t c);

bool uart_poll;

#define TXD_PIN 21
#define RXD_PIN 19
#define FTDI

#define UART_NUM_SLIP UART_NUM_1

#ifdef CP2102
UART &uart =
    UART::create(UART_NUM_0, TXD_PIN, RXD_PIN);  // pins not used,onboard USB
#define BAUDRATE 921600
#endif
#ifdef FTDI
UART &uart = UART::create(UART_NUM_1, TXD_PIN, RXD_PIN);
#define BAUDRATE 1000000
#endif

#define PATTERN_CHR_NUM 1

extern "C" sio_fd_t sio_open(u8_t devnum) {
  return Sio::getInstance().open(devnum);
}

extern "C" u8_t sio_recv(sio_fd_t fd) { return ((Sio *)fd)->recv(); }

extern "C" void sio_send(u8_t c, sio_fd_t fd) { return ((Sio *)fd)->send(c); }

extern "C" u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len) {
  return ((Sio *)fd)->read(data, len);
}

extern "C" u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len) {
  return ((Sio *)fd)->tryread(data, len);
}

extern "C" u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len) {
  return ((Sio *)fd)->write(data, len);
}

extern "C" void sio_read_abort(sio_fd_t fd) {
  return ((Sio *)fd)->read_abort();
}

static void uart_event_task(void *pvParameters) {
  Sio *sio = (Sio *)pvParameters;
  sio->event_task();
}

Sio *Sio::_instance = 0;
/// PPP
u32_t Sio::write(u8_t *data, u32_t len) {
  //  INFO(" SIO write : %d ",len);
  int rc = uart_write_bytes(_uartNum, data, len);
  return (rc < 0) ? 0 : len;
}

/// SLIP
u32_t Sio::read(u8_t *data, u32_t len) {
  return uart_read_bytes(_uartNum, data, len, (portTickType)portMAX_DELAY);
}

void Sio::send(u8_t c) { uart_write_bytes(_uartNum, &c, 1); }

void Sio::event_task() {
  uart_event_t event;
  size_t buffered_size;
  uint8_t *dtmp = (uint8_t *)malloc(_bufferSize);
  INFO(" uart%d task started ", _uartNum);
  for (;;) {
    // Waiting for UART event.
    if (xQueueReceive(_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
      bzero(dtmp, _bufferSize);
      switch (event.type) {
        // Event of UART receving data
        /*We'd better handler data event fast, there would be much more
         data events than other types of events. If we take too much time
         on data event, the queue might be full.*/
        case UART_DATA: {
          /*      int n = uart_read_bytes(_uartNum, dtmp, event.size,
             portMAX_DELAY); if (n < 0) ERROR("uart_read_bytes() failed.");
                _bufferHandler(dtmp, n);*/
          //          INFO("UART_DATA : %d", n);
          break;
        }
        // Event of HW FIFO overflow detected
        case UART_FIFO_OVF:
          INFO("hw fifo overflow");
          // If fifo overflow happened, you should consider adding
          // flow control for your application. The ISR has already
          // reset the rx FIFO, As an example, we directly flush the
          // rx buffer here in order to read more data.
          uart_flush_input(_uartNum);
          xQueueReset(_queue);
          break;
        // Event of UART ring buffer full
        case UART_BUFFER_FULL:
          INFO("ring buffer full");
          // If buffer full happened, you should consider encreasing
          // your buffer size As an example, we directly flush the rx
          // buffer here in order to read more data.
          uart_flush_input(_uartNum);
          xQueueReset(_queue);
          break;
        // Event of UART RX break detected
        case UART_BREAK:
          INFO("uart rx break");
          break;
        // Event of UART parity check error
        case UART_PARITY_ERR:
          WARN("uart parity error");
          break;
        // Event of UART frame error
        case UART_FRAME_ERR:
          WARN("uart frame error");
          break;
        // UART_PATTERN_DET
        case UART_PATTERN_DET: {
          uart_get_buffered_data_len(_uartNum, &buffered_size);
          int pos = uart_pattern_pop_pos(_uartNum);
          if (pos == -1) {
            uart_flush_input(_uartNum);
          } else {
            if (pos > 0) {
              int n = uart_read_bytes(_uartNum, dtmp, pos + 1, 10);
              if (n < 0) {
                ERROR("uart_read_bytes() failed.");
              } else {
                _bufferHandler(dtmp, n);
              }
            }
          }
          break;
        }
        // Others
        default:
          INFO("uart event type: %d", event.type);
          break;
      }
    }
  }
  free(dtmp);
  dtmp = NULL;
  vTaskDelete(NULL);
}

sio_fd_t Sio::open(u8_t devnum) {
  INFO(" Sio::open(%d)  ",devnum);
  _uartNum = devnum > 2 ? UART_NUM_SLIP : devnum;
  _uart_config.data_bits = UART_DATA_8_BITS;
  _uart_config.parity = UART_PARITY_DISABLE;
  _uart_config.stop_bits = UART_STOP_BITS_1;
  _uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  int rc = uart_param_config(_uartNum, &_uart_config);
  if (rc) {
    ERROR(" uart_param_config() failed : %d  ", rc);
    return 0;
  };
  if (_uartNum == UART_NUM_0) {
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  // no CTS,RTS
  } else {
    uart_set_pin(_uartNum, _pinTxd, _pinRxd, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);  // no CTS,RTS
  }
  if (uart_driver_install(_uartNum, _bufferSize, 0, 20, &_queue, 0)) {
    ERROR("uart_driver_install() failed.");
    return 0;
  }
  err_t erc =
      uart_enable_pattern_det_baud_intr(_uartNum, _pattern, 1, 1000, 0, 0);
  if (erc) WARN("uart_enable_pattern_det_baud_intr()=%d ", rc);
  erc = uart_pattern_queue_reset(_uartNum, 20);
  if (erc) WARN("uart_pattern_queue_reset()=%d ", rc);

  std::string taskName;
  string_format(taskName, "uart_event_task_%d", _uartNum);
  xTaskCreate(uart_event_task, taskName.c_str(), 3120, this,
              /*tskIDLE_PRIORITY + 5*/ 20, &_taskHandle);
  INFO(" UART_%d %d baud opened. ", _uartNum, _uart_config.baud_rate);
  return this;
}
