#include <PubSubClient.h>
#include <constants.h>

const char* mqtt_host = MQTT_SERVER;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pwd  = MQTT_PWD;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
bool mqtt_connected = false;

void mqtt_setup() {
  client.setServer(mqtt_host, mqtt_port);
  client.setCallback(mqtt_callback);
  mqtt_connect();
}

void mqtt_handle() {
  if (!client.connected()) {
    mqtt_connected = false;
    mqtt_connect();
  }
  client.loop();
}

void mqtt_connect() {
  int i = 0;
  while (!client.connected()) {
    // Try to reconnect
    client.connect(mqtt_client_id, mqtt_user, mqtt_pwd);
    delay(500);
    //blink(50, 2000);

    // If not connected after 20 sec
    // Seems to be a bug in the PubSubClient librairies
    // which make the esp8266 fail to reconnect to the mqtt broker
    // so we do a hard reboot of the chip
    if (i++ >= 10) {
      while (1); // infinite loop => trigger watchdog
    }
  }
  if (!mqtt_connected) {
    // Was not connected
    client.subscribe(mqtt_topic_cmd);
    client.subscribe(mqtt_topic_state);
    delay(10);
    client.publish("hello/world", mqtt_client_id);
    // ...now we are
    mqtt_connected = true;
  }
}


void mqtt_callback(char* topic_str, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);
  String topic = String(topic_str);

  rdebugDln("[%s] %s", topic.c_str(), message.c_str());
  //Serial.println(String(topic) + " " + message);

  if (topic == mqtt_topic_cmd) {
    ch_callback(topic, message);
  }
  else if (topic == mqtt_topic_state) {
    ch_callback(topic, message);
  }
}
