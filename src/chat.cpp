#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const int PIN_LED = 4;
static const int PIN_VENT = 14;

// Простая URL‑кодировка UTF‑8: ' ' -> '+', ASCII 33..127 (кроме '?' и '=') — как есть,
// остальное байтово кодируем как %HH
static String urlEncodeUtf8(const String &value) {
  String out;
  out.reserve(value.length() * 3);
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t ch = static_cast<uint8_t>(value[i]);
    if (ch == ' ') {
      out += '+';
    } else if (ch > 32 && ch < 128 && ch != '?' && ch != '=') {
      out += static_cast<char>(ch);
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%02X", ch);
      out += '%';
      out += buf;
    }
  }
  return out;
}

// Безопасный HTTPS клиент (без проверки сертификата)
static bool httpsGet(const String &url, String &payload, int &code) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    code = -1;
    return false;
  }
  http.setTimeout(15000);
  http.addHeader("Accept", "application/json");

  code = http.GET();
  if (code > 0) {
    payload = http.getString();
  } else {
    payload = "";
  }
  http.end();
  return code > 0;
}

void runN8nDemo() {
  // Настройка GPIO
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_VENT, OUTPUT);

  // d['свет'].value(1)
  digitalWrite(PIN_LED, HIGH);

  // Подключение к Wi‑Fi (как в примере)
  const char *ssid = "Samsung_IoT";
  const char *password = "IOT5iot5";

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Подключение к Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(300);
      Serial.print(".");
    }
    Serial.println("\nConnection successful");
  }

  String t = String("Щас спою");
  String tEnc = urlEncodeUtf8(t);
  Serial.println(tEnc);

  // GET https://n8n.levandrovskiy.ru/send_message?chat_name=v6&sender=xd&content={t}
  String url1 = String("https://n8n.levandrovskiy.ru/send_message?chat_name=v6&sender=xd&content=") + tEnc;
  String payload1;
  int code1 = 0;
  if (!httpsGet(url1, payload1, code1)) {
    Serial.printf("send_message: HTTP ошибка: %d\n", code1);
  } else {
    Serial.printf("send_message: статус: %d\n", code1);
    Serial.println(payload1);
  }

  // data = GET https://n8n.levandrovskiy.ru/get_recent_messages?chat_name=v6&limit=1&sender=user
  String url2 = String("https://n8n.levandrovskiy.ru/get_recent_messages?chat_name=v6&limit=1&sender=user");
  String payload2;
  int code2 = 0;
  if (!httpsGet(url2, payload2, code2)) {
    Serial.printf("get_recent_messages: HTTP ошибка: %d\n", code2);
    return;
  }

  Serial.printf("get_recent_messages: статус: %d\n", code2);
  Serial.println(payload2);

  // Парсим JSON и печатаем content
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload2);
  if (err) {
    Serial.print("JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  // Поддержка двух форматов:
  // 1) Объект: { "content": "..." }
  // 2) Массив объектов: [ { "content": "...", ... }, ... ]
  if (doc["content"].is<String>()) {
    Serial.println(doc["content"].as<String>());
    return;
  }

  if (doc.is<JsonArray>()) {
    JsonArray arr = doc.as<JsonArray>();
    if (!arr.isNull() && arr.size() > 0) {
      JsonVariant first = arr[0];
      if (!first.isNull() && first["content"].is<String>()) {
        Serial.println(first["content"].as<String>());
        return;
      }
    }
  }

  Serial.println("Поле 'content' не найдено в ответе");
}

void setup() {
  Serial.begin(9600);
  delay(300);
}

void loop() {
  runN8nDemo();
  delay(10000);
}