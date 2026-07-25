#ifndef HEADERS_H
#define HEADERS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_ADS1X15.h>
#include "PCF8575.h"

#include "Config.h"
#include "Machine_State.h"
#include "GlobalVars.h"   // <-- ora include Globals.h + le variabili extra

#include "OTA_Manager.h"
#include "CNC_Controller.h"
#include "Input_Handler.h"
#include "Display_Manager.h"
#include "Safety_Manager.h"
#include "LED_Manager.h"
#include "ButtonLedMap.h"
#include "Button_Handler.h"
#include "PCF8575_Manager.h"
#include "ESPNow_Manager.h"   // la classe ESPNowManager

#endif