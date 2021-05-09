#include "syslog.h"

#include <Log.h>
#include <esp_log.h>
#include <time.h>

static Syslog *me = 0;
static std::string logLine;

int espLog(const char *fmt, va_list va) {
  string_format(logLine, fmt, va);
  if (me) me->simple(logLine.c_str(), logLine.length());
  return 0;
}

Syslog::Syslog(Thread &thread) : Actor(thread) {}

bool Syslog::init(const char *ipAddr, int port) {
  _port = port;
  if (ipaddr_aton(ipAddr, &_addr)) {
    _conn = netconn_new(NETCONN_UDP);
    if (_conn != NULL) {
      if (netconn_connect(_conn, &_addr, _port) == ERR_OK) {
        me = this;
        logger.writer([](char *start, uint32_t length) {
          if (me) me->simple(start, length);
        });
        esp_log_set_vprintf(espLog);
        return true;
      } else {
        WARN("netconn_connect failed ");
      }
    } else {
      WARN("netconn_new failed");
    }
  } else {
    WARN(" ipaddr_aton failed ");
  }
  _logRecord.resize(512);
  return false;
}

void Syslog::log(const char *line, int length) {
  int severity = SEVERITY_INFO;
  time_t rawtime;
  struct tm *timeinfo;
  char buffer[80];

  uint32_t msec = Sys::millis() % 1000;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

  string_format(_logRecord, "<%d>1 %s.%03dZ %s app pid mid - BOM %s",
                FACILITY_USER | severity, buffer, msec, Sys::hostname(),
                line);  // forma
                        //  INFO("%s", _logRecord.c_str());
  _buf = netbuf_new();
  _data = netbuf_alloc(_buf, _logRecord.length());
  memcpy(_data, _logRecord.c_str(), _logRecord.length());
  err_t err = netconn_send(_conn, _buf);
  if (err) INFO(" netconn_send() %d", err);

  //  err_t err = netconn_sendto(_conn, _buf, &_addr, _port);
  // if (err) INFO(" netconn_sendto() %d", err);
  netbuf_delete(_buf);
}

void Syslog::simple(const char *line, int length) {
  _buf = netbuf_new();
  _data = netbuf_alloc(_buf, length);
  memcpy(_data, line, length);
  netconn_send(_conn, _buf);
  netbuf_delete(_buf);
}
