#include "syslog.h"

#include <time.h>

bool Syslog::init(const char *ipAddr, int port) {
  _port = port;
  if (ipaddr_aton(ipAddr, &_addr)) {
    _conn = netconn_new(NETCONN_UDP);
    if (_conn != NULL) {
      if (netconn_connect(_conn, &_addr, _port) == ERR_OK) {
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
  return false;
}

void Syslog::log(const char *line, int length) {
  int severity = SEVERITY_INFO;
  time_t now;
  string_format(_logRecord, "<%d>1 %.24s %s app pid mid - BOM %s",
                FACILITY_USER | severity, "2021-05-08T01:11:20.003Z",
                Sys::hostname(),
                line);  // forma
  INFO("%s", _logRecord.c_str());
  _buf = netbuf_new();
  _data = netbuf_alloc(_buf, _logRecord.length());
  memcpy(_data, _logRecord.c_str(), _logRecord.length());
  err_t err = netconn_send(_conn, _buf);
  if (err) INFO(" netconn_send() %d", err);
  err = netconn_sendto(_conn, _buf, &_addr, _port);
  if (err) INFO(" netconn_sendto() %d", err);
  netbuf_delete(_buf);
}
