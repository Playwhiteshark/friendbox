#include <Arduino.h>
#include "app/App.h"

friendbox::app::App app;

void setup() { app.begin(); }
void loop() { app.update(); }
