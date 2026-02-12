#include <Arduino.h>
#include "AppController.h"

static AppController app;

void setup() {
  app.begin();
}

void loop() {
  app.loop();
}
