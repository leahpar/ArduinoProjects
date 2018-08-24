#ifndef CALLBACKFUNCTION_H
#define CALLBACKFUNCTION_H

#include <Arduino.h>

typedef bool (*CallbackFunction) (String device_id, String state);

#endif
