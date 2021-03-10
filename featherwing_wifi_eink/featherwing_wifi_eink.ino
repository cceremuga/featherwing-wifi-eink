#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include "Adafruit_ThinkInk.h"
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

#define EPD_CS 9
#define EPD_DC 10
#define SRAM_CS 6
#define EPD_RESET -1
#define EPD_BUSY -1
#define SLEEP_MINUTES 3
#define NEWLINE "\r\n"

// https://www.adafruit.com/product/4195
ThinkInk_213_Mono_B72 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

WiFiClient client;

void setup() {
  // https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/using-the-wifi-module
  WiFi.setPins(8, 7, 4, 2);

  Serial.begin(9600);

  display.begin(THINKINK_MONO);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not connected.");
    while (true);
  }
}

void loop() {
  Serial.println("Wakeup.");

  wifiConnect();

  // Some connection details for the display.
  String ip = getIP();

  // Grab example JSON.
  String jsonTime = String(getJsonTime());

  // Print everything + current runtime in minutes.
  eInk(ip + NEWLINE + jsonTime + NEWLINE + \
       String(millisecondsToMinutes(millis())) +  " minutes");

  wifiDisconnect();

  // It's recommended to not refresh the display more frequently.
  Serial.print("Sleeping ");
  Serial.print(SLEEP_MINUTES);
  Serial.println(" minutes.");
  delay(minutesToMilliseconds(SLEEP_MINUTES));
}

void wifiDisconnect() {
  Serial.print("Disconnecting from ");
  Serial.println(ssid);
  WiFi.disconnect();
}

void wifiConnect() {
  status = WiFi.status();
  Serial.print("WiFi status: ");
  Serial.println(status);

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    delay(10000);
  }
}

long getJsonTime() {
  client.setTimeout(10000);
  if (!client.connect("arduinojson.org", 80)) {
    Serial.println(F("Web connection failed."));
    return 0;
  }

  Serial.println(F("Connected to web server!"));

  // Send HTTP request
  client.println(F("GET /example.json HTTP/1.0"));
  client.println(F("Host: arduinojson.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    client.stop();
    return 0;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  // It should be "HTTP/1.0 200 OK" or "HTTP/1.1 200 OK"
  if (strcmp(status + 9, "200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    client.stop();
    return 0;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    client.stop();
    return 0;
  }

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    client.stop();
    return 0;
  }

  // Disconnect
  client.stop();

  return doc["time"].as<long>();
}

String getIP() {
  return ipToString(WiFi.localIP());
}

String ipToString(const IPAddress& ipAddress) {
  return String(ipAddress[0]) + "." + \
         String(ipAddress[1]) + "." + \
         String(ipAddress[2]) + "." + \
         String(ipAddress[3]);
}

void eInk(String text) {
  Serial.println(text);

  display.clearBuffer();
  display.setTextSize(2);
  display.setCursor(0, 1);
  display.setTextColor(EPD_BLACK);
  display.print(text);
  display.display();
}

int minutesToMilliseconds(int minutes) {
  return minutes * 60 * 1000;
}

long millisecondsToMinutes(int milliseconds) {
  return long(milliseconds) / 1000.0 / 60.0;
}
