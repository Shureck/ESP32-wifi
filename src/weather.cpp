#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Samsung_IoT";
const char* password = "IOT5iot5";

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://goweather.herokuapp.com/weather/Moscow");
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      Serial.println("Response:");
      Serial.println(payload);

      // Парсинг JSON
      DynamicJsonDocument data(1024);
      deserializeJson(data, payload);

      const char* description = data["description"];
      const char* temperature = data["temperature"];
      Serial.printf("Погода: %s, Температура: %s\n", description, temperature);

      // Прогноз на 3 дня
      JsonArray forecast = data["forecast"];
      for (int i = 0; i < forecast.size(); i++) {
        Serial.printf("День %s: %s, ветер %s\n",
                      forecast[i]["day"].as<const char*>(),
                      forecast[i]["temperature"].as<const char*>(),
                      forecast[i]["wind"].as<const char*>());
      }
    } else {
      Serial.printf("Ошибка HTTP: %d\n", httpResponseCode);
    }
    http.end();
  }
}

void loop() {}