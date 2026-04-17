#include <Arduino.h>
#include "App/App.h"
namespace {App app;}
void setup() {app.init();}
void loop() {app.process();}
