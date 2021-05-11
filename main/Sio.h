#ifndef _SIO_H_
#define _SIO_H_
#include <deque>

#include "Hardware.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "limero.h"
#include "lwip/opt.h"
#include "lwip/sio.h"
#include "netif/slipif.h"

typedef void (*BufferHandler)(uint8_t *data, uint32_t length);


class Sio {
  uart_port_t _uartNum;
  uart_config_t _uart_config;
  uint32_t _pinTxd;
  uint32_t _pinRxd;
  static Sio *_instance;
  uint32_t _bufferSize;
  uint8_t _pattern;
  std::deque<uint8_t> _rxdBuf;
  BufferHandler _bufferHandler;

  QueueHandle_t _queue = 0;
  TaskHandle_t _taskHandle = 0;

  Sio(uint32_t bufferSize, uint8_t pattern, uint32_t pinTxd, uint32_t pinRxd,uint32_t baudrate,
      BufferHandler bufferHandler) {
    _pattern = pattern;
    _bufferSize = bufferSize;
    _pinTxd = pinTxd;
    _pinRxd = pinRxd;
    _bufferHandler = bufferHandler;
    _uart_config.baud_rate=baudrate;
  }

 public:
  void event_task();

  static void createInstance(uint32_t bufferSize, uint8_t pattern,
                             uint32_t pinTxd, uint32_t pinRxd,uint32_t baudrate,
                             BufferHandler bufferHandler) {
    _instance = new Sio(bufferSize, pattern, pinTxd, pinRxd, baudrate,bufferHandler);
  }

  static Sio &getInstance() { return *_instance; };

  sio_fd_t open(u8_t devnum);
  void send(u8_t c);
  u8_t recv();
  u32_t read(u8_t *data, u32_t len);
  u32_t tryread(u8_t *data, u32_t len);
  u32_t write(u8_t *data, u32_t len);
  void read_abort();
};
#endif