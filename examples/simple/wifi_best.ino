#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#define TRIGGER_PIN 0  // GPIO0 (often the BOOT button)

WiFiManager wifiManager;

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  delay(2000);
  
  // Reset settings - for testing (comment out for production)
  // wifiManager.resetSettings();
  
  // Set timeout for config portal (180 seconds)
  wifiManager.setConfigPortalTimeout(180);
  
  // Set callback for when entering config mode
  wifiManager.setAPCallback(configModeCallback);
  
  // Try to connect, if it fails, start config portal
  if (!wifiManager.autoConnect("ESP32-Setup")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.restart();
  }
  
  // If you get here, you're connected!
  Serial.println("Connected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check WiFi connection and reconnect if needed
  if (digitalRead(TRIGGER_PIN) == LOW) {
    delay(50);  // Debounce
    if (digitalRead(TRIGGER_PIN) == LOW) {
      Serial.println("Button pressed - starting config portal");
      wifiManager.startConfigPortal("ESP32-Setup");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, attempting reconnection...");
    WiFi.reconnect();
    delay(5000);  // Wait 5 seconds before checking again
  }
  
  // Your main code here
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("AP Name: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
}
