
#include <Log.h>
#include <MqttClient.h>

void MqttClient::init(const char* mqttIp,uint32_t port) {
  err_t rc;
  _mqtt_port=port;
  ipaddr_aton(mqttIp, &_mqtt_ip);
  mqtt_client = mqtt_client_new();
  mqtt_client_info.client_id = Sys::hostname();
  // !! Example code on Savannah site has bad order of calls , see :
  // http://savannah.nongnu.org/bugs/?58406
  rc = mqtt_client_connect(
      mqtt_client, &_mqtt_ip, _mqtt_port, mqtt_connection_cb,
      LWIP_CONST_CAST(void*, &mqtt_client_info), &mqtt_client_info);
  if (rc != ERR_OK) WARN("MQTT connect failed");
  mqtt_set_inpub_callback(mqtt_client, mqtt_incoming_publish_cb,
                          mqtt_incoming_data_cb,
                          LWIP_CONST_CAST(void*, &mqtt_client_info));
}

void MqttClient::mqtt_incoming_data_cb(void* arg, const u8_t* data, u16_t len,
                                       u8_t flags) {
  const struct mqtt_connect_client_info_t* client_info =
      (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(data);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" data cb: len %d, flags %d\n",
                      client_info->client_id, (int)len, (int)flags));
}

void MqttClient::mqtt_incoming_publish_cb(void* arg, const char* topic,
                                          u32_t tot_len) {
  const struct mqtt_connect_client_info_t* client_info =
      (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" publish cb: topic %s, len %d\n",
                      client_info->client_id, topic, (int)tot_len));
}

void MqttClient::mqtt_request_cb(void* arg, err_t err) {
  const struct mqtt_connect_client_info_t* client_info =
      (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n",
                      client_info->client_id, (int)err));
}

void MqttClient::mqtt_connection_cb(mqtt_client_t* client, void* arg,
                                    mqtt_connection_status_t status) {
  const struct mqtt_connect_client_info_t* client_info =
      (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" connection cb: status %d\n",
                      client_info->client_id, (int)status));

  if (status == MQTT_CONNECT_ACCEPTED) {
    mqtt_sub_unsub(client, "src/#", 0, mqtt_request_cb,
                   LWIP_CONST_CAST(void*, client_info), 1);
    mqtt_sub_unsub(client, "topic_qos0", 0, mqtt_request_cb,
                   LWIP_CONST_CAST(void*, client_info), 1);
  }
}

void MqttClient::publish_request_cb(void* arg, err_t err) {
  // MqttClient* self = (MqttClient*)arg;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  //  INFO(" publish request callback : %d self : %X ", err,self);
}

void MqttClient::publish(std::string& topic, std::string& message) {
  mqtt_publish(mqtt_client, topic.c_str(), message.c_str(), message.length(), 0,
               0, publish_request_cb, this);
}
