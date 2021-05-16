#include <Hardware.h>
#include <Mqtt.h>
#include <MqttWifi.h>
#include <limero.h>

#define SLIP

#ifdef SLIP
#include <Sio.cpp>
extern bool SlipInit();
#endif

#ifdef PPP
extern bool PppInit();
#endif

#include "MqttClient.h"
#include "Syslog.h"

MqttClient mqtt;

Log logger(1024);
Thread mqttThread("mqtt");
Syslog syslog(mqttThread);
// MqttWifi mqtt(mqttThread);

ValueSource<std::string> systemBuild("NOT SET");
ValueSource<std::string> systemHostname("NOT SET");
ValueSource<bool> systemAlive = true;
LambdaSource<uint32_t> systemHeap([]() { return Sys::getFreeHeap(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
Poller poller(mqttThread);
bool mqttConnected = false;

extern "C" void app_main(void) {
  INFO(" Starting build : %s ", __DATE__);

#ifdef SLIP
  if (SlipInit())
#endif
#ifdef PPP
    if (PppInit())
#endif
    {
      mqttThread.start();
      vTaskDelay(100);
      for (int i = 0; i < DNS_MAX_SERVERS; i++) {
        const ip_addr_t *pdns = dns_getserver(i);
        INFO(" DNS [%d] = %s ", i, ipaddr_ntoa(pdns));
      }
      //      syslog.init("192.168.1.1", 514);
    };
  //  mqtt.wifiConnected.on(true);
  /* poller.connected = true;
   //-----------------------------------------------------------------  SYS
   props poller >> systemUptime >> mqtt.toTopic<uint64_t>("system/upTime");
   poller >> systemHeap >> mqtt.toTopic<uint32_t>("system/heap");
   poller >> systemHostname >> mqtt.toTopic<std::string>("system/hostname");
   poller >> systemBuild >> mqtt.toTopic<std::string>("system/build");
   poller >> systemAlive >> mqtt.toTopic<bool>("system/alive");*/
  while (true) {
    INFO(" INFO log ! %ld", Sys::millis());
    if (mqttConnected) {
      std::string message = std::to_string(Sys::millis());
      std::string topic = "src/ESP/system/upTime";
      LOCK_TCPIP_CORE();
      mqtt.publish(topic, message);
      UNLOCK_TCPIP_CORE();
    }
    ip_addr_t addr;
    err_t rc = dns_gethostbyname(
        "limero.ddns.net", &addr,
        [](const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
          INFO(" dns_gethostname callback  %s : %s ", name,
               ipaddr ? ipaddr_ntoa(ipaddr) : "<not found>");
          if (!mqttConnected && ipaddr != 0) {
            mqtt.init(*ipaddr);
            mqttConnected = true;
          }
        },
        0);
    INFO(" err %d : dns_gethostbyname()", rc);
    vTaskDelay(100);
    dns_tmr();
  }
}