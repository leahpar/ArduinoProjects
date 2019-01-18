
#include <NTPClient.h> // doesn't compile if not included here :/

#include <constants.h>


const char* _host = "node-elec"; // .local

String device_name = "Chauffeau";

const char* mqtt_client_id   = "ArduinoTest";
const char* mqtt_topic_data  = "test/test"; //"sensor/elec/appart";
const char* mqtt_topic_cmd   = "appart/chauffeau/cmd";
const char* mqtt_topic_state = "appart/chauffeau/state";

// https://github.com/JoaoLopesF/RemoteDebug
#include "RemoteDebug.h"
RemoteDebug Debug;

