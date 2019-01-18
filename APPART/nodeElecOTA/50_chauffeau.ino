#include <ESP8266WiFi.h>

bool currentHC = false;
bool ch_relay_on = false;

String ch_state = "AUTO";
String ch_cmd   = "NONE";
unsigned long ch_on_timer = 0;
unsigned long ch_off_timer = 0;

// Timout ON/OFF manuel
unsigned long ch_on_timout = 30 * 1000;
unsigned long ch_off_timout = 30 * 1000;


void ch_setup() {
    // default cmd
    ch_cmd = isHC() ? "HC" : "HP";
}


void ch_handle() {
    if (isHC() && !currentHC) {
        // Passage HP à HC
        ch_cmd = "HC";
        ch_handle_cmd();
        currentHC = true;
        rdebugIln("Passage HP à HC");
    }
    if (isHP() && currentHC) {
        // Passage HC à HP
        ch_cmd = "HP";
        ch_handle_cmd();
        currentHC = false;
        rdebugIln("Passage HP à HC");
    }
    if (ch_on_timer > 0 && millis() - ch_on_timer > ch_on_timout) {
        // Fin de l'allumage, AUTO ou MANUAL
        ch_on_timer = 0;
        ch_cmd = isHC() ? "HC" : "HP";
        rdebugIln("Fin timer ON");
    }
    if (ch_off_timer > 0 && millis() - ch_off_timer > ch_off_timout) {
        // fin de l'extinction.
        ch_off_timer = 0;
        ch_cmd = isHC() ? "HC" : "HP";
        rdebugIln("Fin timer OFF");
    }
    if (ch_cmd != "NONE") {
        // Quelque chose à faire
        ch_handle_cmd();
    }
}

bool isHPEDF() {
    return (timeClient.getHours() > 8   && timeClient.getHours() < 23)
        || (timeClient.getHours() == 7  && timeClient.getMinutes() >= 30)
        || (timeClient.getHours() == 23 && timeClient.getMinutes() <= 30)
    ;
}
bool isHCEDF() {
    return !isHPEDF();
}

bool isHP() {
    return !isHC();
}
bool isHC() {
    return (timeClient.getHours() >= 14 && timeClient.getHours() <= 15);
}

void turnOnRelay(String src) {
    rdebugIln("Relay ON (%s)", src.c_str());
    ch_relay_on = true;
}

void turnOffRelay(String src) {
    rdebugIln("Relay OFF (%s)", src.c_str());
    ch_relay_on = false;
}


void ch_callback(String topic, String message) {
    rdebugDln("[%s] %s", topic.c_str(), message.c_str());
    if (topic == mqtt_topic_cmd) {
        ch_cmd = message;
    }
    else if (topic == mqtt_topic_state) {
        ch_state = message;
    }
}

void ch_handle_cmd() {
    // ch_cmd = NONE - HP - HC - ON - OFF
    // ch_state = AUTO - MANUAL

    rdebugIln("CMD = %s ; STATE = %s", ch_cmd.c_str(), ch_state.c_str());

    if (ch_state == "AUTO") {
        if (ch_cmd == "HP") {
            turnOffRelay("AUTO HP");
        }
        else if (ch_cmd == "HC") {
            turnOnRelay("AUTO HC");
        }
        else if (ch_cmd == "ON") {
            turnOnRelay("AUTO ON");
            // Timer if HP
            if (isHP()) ch_on_timer = millis();
        }
        else if (ch_cmd == "OFF") {
            turnOffRelay("AUTO OFF");
            // Timer if HC
            if (isHC()) ch_off_timer = millis();
        }
    }
    else if (ch_state == "MANUAL") {
        if (ch_cmd == "OFF") {
            turnOffRelay("MANUAL OFF");
        }
        else if (ch_cmd == "ON") {
            turnOnRelay("MANUAL ON");
            ch_on_timer = millis();
        }
        else if (ch_cmd == "HP") {
            turnOffRelay("MANUAL HP");
        }
        else if (ch_cmd == "HC") {
            turnOffRelay("MANUAL HC");
        }

    }
    ch_cmd = "NONE";
}
