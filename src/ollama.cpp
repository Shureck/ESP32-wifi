#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// === Wi-Fi ===
const char* ssid = "Samsung_IoT";
const char* password = "IOT5iot5";

// === Ollama ===
const char* OLLAMA_HOST = "192.168.1.87";
const int OLLAMA_PORT = 11434;
const char* MODEL_NAME = "qwen2.5:7b";

float temperature = 22.5;
int humidity = 35;

void sendToOllama() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://" + String(OLLAMA_HOST) + ":" + String(OLLAMA_PORT) + "/api/generate";
  Serial.print("HTTP URL: ");
  Serial.println(url);
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());

  // Быстрая проверка TCP-соединения
  WiFiClient testClient;
  if (!testClient.connect(OLLAMA_HOST, OLLAMA_PORT)) {
    Serial.println("TCP подключение не установлено (сервер недоступен или порт закрыт)");
    return;
  }
  testClient.stop();

  WiFiClient client;
  http.begin(client, url);
  http.setTimeout(15000); // увеличенный таймаут чтения/записи

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  String prompt = "Температура: " + String(temperature) + "°C, влажность: " + String(humidity) + "%. "
    "Ты — помощник умного дома. Ответь ТОЛЬКО валидным JSON без пояснений и без ```. Формат: "
    "{\"recommendations\":[\"команда1\",\"команда2\"],\"reason\":\"...\",\"comfort_level\":\"низкий/средний/высокий\"}.";

  // Формируем корректный JSON через ArduinoJson (автоматически экранирует спецсимволы)
  DynamicJsonDocument bodyDoc(1024);
  bodyDoc["model"] = MODEL_NAME;
  bodyDoc["prompt"] = prompt;
  bodyDoc["stream"] = false;
  JsonObject options = bodyDoc.createNestedObject("options");
  options["temperature"] = 0.3;

  String jsonBody;
  serializeJson(bodyDoc, jsonBody);

  int code = http.POST(jsonBody);

  if (code == 200) {
    String response = http.getString();

    // Парсинг ответа Ollama (поле "response" содержит текст)
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, response);

    if (!err && doc.containsKey("response")) {
      String content = doc["response"].as<String>();

      // Попытка распарсить JSON из ответа модели
      DynamicJsonDocument resultDoc(1024);
      DeserializationError jsonErr = deserializeJson(resultDoc, content);

      if (!jsonErr) {
        Serial.println("\nКоманды от модели:");
        for (String cmd : resultDoc["recommendations"].as<JsonArray>()) {
          Serial.println(" → " + cmd);
        }
        Serial.println("Причина: " + resultDoc["reason"].as<String>());
        Serial.println("Комфорт: " + resultDoc["comfort_level"].as<String>());
      } else {
        Serial.println("\nМодель не вернула чистый JSON:");
        Serial.println(content);
      }
    } else {
      Serial.println("Ошибка парсинга ответа Ollama");
      Serial.println(response);
    }
  } else {
    Serial.printf("HTTP ошибка: %d\n", code);
    Serial.print("WiFi.status(): ");
    Serial.println((int)WiFi.status());
    // Печатаем тело запроса для отладки, если сервер вернул 400 (скорее всего проблема в JSON)
    if (code == 400) {
      Serial.println("Отправленный JSON:");
      Serial.println(jsonBody);
    }
    Serial.println(http.getString());
  }

  http.end();
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Подключение к Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nПодключено!");
}

void loop() {
  sendToOllama();
  delay(30000); // раз в 30 сек
}