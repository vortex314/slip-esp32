#ifndef SYSLOG_H
#define SYSLOG_H
#include <Log.h>
#include <lwip/api.h>
#include <lwip/ip_addr.h>
#include <lwip/netbuf.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <stdint.h>

#include <string>

/* facility codes */
#define FACILITY_KERN (0 << 3)   /* kernel messages */
#define FACILITY_USER (1 << 3)   /* random user-level messages */
#define FACILITY_MAIL (2 << 3)   /* mail system */
#define FACILITY_DAEMON (3 << 3) /* system daemons */
#define FACILITY_AUTH (4 << 3)   /* security/authorization messages */
#define FACILITY_SYSLOG (5 << 3) /* messages generated internally by syslogd \
                                  */
#define FACILITY_LPR (6 << 3)    /* line printer subsystem */
#define FACILITY_NEWS (7 << 3)   /* network news subsystem */
#define FACILITY_UUCP (8 << 3)   /* UUCP subsystem */
#define FACILITY_CRON (9 << 3)   /* clock daemon */
#define FACILITY_AUTHPRIV \
  (10 << 3)                    /* security/authorization messages (private) */
#define FACILITY_FTP (11 << 3) /* ftp daemon */

/* other codes through 15 reserved for system use */
#define FACILITY_LOCAL0 (16 << 3) /* reserved for local use */
#define FACILITY_LOCAL1 (17 << 3) /* reserved for local use */
#define FACILITY_LOCAL2 (18 << 3) /* reserved for local use */
#define FACILITY_LOCAL3 (19 << 3) /* reserved for local use */
#define FACILITY_LOCAL4 (20 << 3) /* reserved for local use */
#define FACILITY_LOCAL5 (21 << 3) /* reserved for local use */
#define FACILITY_LOCAL6 (22 << 3) /* reserved for local use */
#define FACILITY_LOCAL7 (23 << 3) /* reserved for local use */

// SEVERITY
#define SEVERITY_EMERG 0   /* system is unusable */
#define SEVERITY_ALERT 1   /* action must be taken immediately */
#define SEVERITY_CRIT 2    /* critical conditions */
#define SEVERITY_ERR 3     /* error conditions */
#define SEVERITY_WARNING 4 /* warning conditions */
#define SEVERITY_NOTICE 5  /* normal but significant condition */
#define SEVERITY_INFO 6    /* informational */
#define SEVERITY_DEBUG 7   /* debug-level messages */

class Syslog {
  struct netconn *_conn;
  ip_addr_t _addr;
  int _port;
  struct netbuf *_buf;
  void *_data;
  std::string _logRecord;

 public:
  bool init(const char *ip, int port);
  void log(const char *line, int length);
};

#endif