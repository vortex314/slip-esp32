#include <Hardware.h>
#include <Mqtt.h>
#include <MqttWifi.h>
#include <limero.h>

#define PPP

#ifdef SLIP
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

extern "C" void app_main(void) {
  INFO(" Starting build : %s ", __DATE__);

#ifdef SLIP
  if (SlipInit())
#endif
#ifdef PPP
    if (PppInit())
#endif
    {
      //     syslog.init("192.168.1.1", 514);
      mqtt.init("192.168.1.1");
    };
  //  mqtt.wifiConnected.on(true);
  /* poller.connected = true;
   //-----------------------------------------------------------------  SYS
   props poller >> systemUptime >> mqtt.toTopic<uint64_t>("system/upTime");
   poller >> systemHeap >> mqtt.toTopic<uint32_t>("system/heap");
   poller >> systemHostname >> mqtt.toTopic<std::string>("system/hostname");
   poller >> systemBuild >> mqtt.toTopic<std::string>("system/build");
   poller >> systemAlive >> mqtt.toTopic<bool>("system/alive");*/
  mqttThread.start();
  vTaskDelay(10);
  while (true) {
    INFO(" INFO log ! %ld", Sys::millis());
    std::string message = std::to_string(Sys::millis());
    std::string topic = "src/ESP/system/upTime";
    mqtt.publish(topic, message);
    vTaskDelay(10);
  }
}