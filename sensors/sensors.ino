// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "private.h"

DHT_Unified dht1(13, DHT22);
DHT_Unified dht2(12, DHT22);

uint32_t delayMS = 5000;

struct dht_reading_t {
  float temp; // f
  float humidity;
};

void setup_dht() {
  dht1.begin();
  dht2.begin();
  sensor_t sensor;
  dht1.temperature().getSensor(&sensor);
}

void setup_wifi() {
  // // Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  // Connecting to WiFi...
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  // Loop continuously while WiFi is not connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
 
  // Connected to WiFi
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  setup_dht();
  setup_wifi();
  Serial.println(F("------------------------------------"));
}

dht_reading_t readDHT(DHT_Unified* dht) {
  // Get temperature event and print its value.
  sensors_event_t event;
  dht_reading_t result;

  dht->temperature().getEvent(&event);
  result.temp = isnan(event.temperature) ? event.temperature : event.temperature * 1.8 + 32;
  dht->humidity().getEvent(&event);
  result.humidity = event.relative_humidity;
  return result;
}

void printReading(dht_reading_t* reading) {
  Serial.print("Temp: ");
  Serial.print(reading->temp);
  Serial.println(" F");
  Serial.print("Humidity: ");
  Serial.print(reading->humidity);
  Serial.println("%");
}

void loop() {
  // Delay between measurements.
  delay(delayMS);

  dht_reading_t r1 = readDHT(&dht1);
  dht_reading_t r2 = readDHT(&dht2);

  Serial.println("--- DHT 1 ---");
  printReading(&r1);
  Serial.println("--- DHT 2 ---");
  printReading(&r2);
  Serial.println("------------");
}