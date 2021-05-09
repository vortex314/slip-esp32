#ifndef MQTT_CLIENT_LWIP
#define MQTT_CLIENT_LWIP

#include <lwip/api.h>
#include <mqtt.h>

class MqttClient {
  ip_addr_t _mqtt_ip;
  int _mqtt_port = 1883;
  mqtt_client_t* mqtt_client;
  struct mqtt_connect_client_info_t mqtt_client_info = {"test",  // client Id
                                                        NULL,    /* user */
                                                        NULL,    /* pass */
                                                        100,  /* keep alive */
                                                        NULL, /* will_topic */
                                                        NULL, /* will_msg */
                                                        0,    /* will_qos */
                                                        0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
                                                        ,
                                                        NULL
#endif
  };
 public:
  void init(const char* mqttIp);

  static void mqtt_incoming_data_cb(void* arg, const u8_t* data, u16_t len,
                                    u8_t flags);

  static void mqtt_incoming_publish_cb(void* arg, const char* topic,
                                       u32_t tot_len);

  static void mqtt_request_cb(void* arg, err_t err);

  static void mqtt_connection_cb(mqtt_client_t* client, void* arg,
                                 mqtt_connection_status_t status);

  void publish(std::string& topic, std::string& message);
  static void publish_request_cb(void*,err_t) ;
};

#endif
