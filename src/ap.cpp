#include <WiFi.h>

void setup() {
  Serial.begin(9600);
  // Настройка точки доступа
  WiFi.softAP("ESP_IoT", "ESP_IoT_2025");
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

  Serial.println("AP started");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {}