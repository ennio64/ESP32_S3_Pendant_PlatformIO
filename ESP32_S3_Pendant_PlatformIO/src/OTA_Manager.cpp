#include "Headers.h"

WebServer OTAManager::server(80);
HTTPUpdateServer OTAManager::httpUpdater;
bool OTAManager::otaEnabled = false;
unsigned long OTAManager::otaStartTime = 0;

void OTAManager::setup() {
  Serial.println("🌐 Configuring OTA Access Point...");
  
  // Configure as Access Point
  WiFi.mode(WIFI_AP);
  
  // Start AP
  bool apStarted = WiFi.softAP("MyESPfamily", "12345678");
  delay(1000);
  
  if (!apStarted) {
    Serial.println("❌ Failed to start Access Point");
    return;
  }

  // Configure AP IP
  IPAddress localIP(192, 168, 5, 1);
  IPAddress gateway(192, 168, 5, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(localIP, gateway, subnet);

  Serial.println("✅ Access Point Started!");
  Serial.println("📶 SSID: MyESPfamily");
  Serial.println("🔑 Password: 12345678");
  Serial.print("📱 IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("📡 MAC: ");
  Serial.println(WiFi.softAPmacAddress());

  // Configure update server
  httpUpdater.setup(&server);
  server.begin();

  otaEnabled = true;
  otaStartTime = millis();
  
  Serial.println("🌐 OTA Update Ready:");
  Serial.println("   - Web: http://192.168.5.1/update");
  Serial.println("   - Connect to: MyESPfamily / 12345678");
}

void OTAManager::update() {
  if (otaEnabled) {
    server.handleClient();
    
    // Blink LED during OTA
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    
    if (millis() - lastBlink > 500) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
      lastBlink = millis();
    }

    // Show AP stats occasionally
    static unsigned long lastStats = 0;
    if (millis() - lastStats > 10000) {
      Serial.printf("📊 Connected clients: %d\n", WiFi.softAPgetStationNum());
      lastStats = millis();
    }
  }
}

bool OTAManager::isOTAEnabled() {
  return otaEnabled;
}

void OTAManager::disableOTA() {
  if (otaEnabled) {
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    otaEnabled = false;
    Serial.println("🔒 OTA disabled - AP turned off");
  }
}

String OTAManager::getConnectionInfo() {
  if (!otaEnabled) return "OTA Disabled";
  
  return "SSID: MyESPfamily\nIP: 192.168.5.1\nPass: 12345678";
}

unsigned long OTAManager::getOTAUptime() {
  return otaEnabled ? (millis() - otaStartTime) : 0;
}