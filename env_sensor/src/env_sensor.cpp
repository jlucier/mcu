#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "private.h"

#define PROM_NAMESPACE "drybox"
#define SENSE_EVERY 10000
#define HTTP_METRICS_ENDPOINT "/metrics"
#define HTTP_BLINK_ENDPOINT "/blink"
#define HTTP_SERVER_PORT 80

#define DHT1_PIN D7
#define DHT2_PIN D6

DHT dht_ext(DHT1_PIN, DHT22);
DHT dht_int(DHT2_PIN, DHT22);
ESP8266WebServer http_server(HTTP_SERVER_PORT);

struct dht_reading_t {
    float temp; // c
    float humidity;
    float heat_index;
    bool success;
};

struct full_reading_t {
    dht_reading_t interior;
    dht_reading_t exterior;
    uint64_t millis = 0;
    bool success = false;
};

dht_reading_t sense(DHT& dht) {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  return dht_reading_t{
    t,
    h,
    dht.computeHeatIndex(t, h, false),
    !(isnan(t) || isnan(h)),
  };
}

full_reading_t sense_all() {
  auto interior = sense(dht_int);
  auto exterior = sense(dht_ext);
  return full_reading_t{
    interior,
    exterior,
    millis(),
    interior.success && exterior.success,
  };
}

void esp8266_main_led(bool val) {
  // high and low are backwards?
  digitalWrite(LED_BUILTIN, val ? LOW : HIGH);
}

// endpoints

void handle_http_root() {
  static size_t const BUFSIZE = 256;
  static char const *response_template =
    "Prometheus ESP8266 Sensor Exporter.\n"
    "\n"
    "Referenced heavily from: https://github.com/HON95/prometheus-esp8266-dht-exporter\n"
    "\n"
    "Usage: %s\n";
  char response[BUFSIZE];
  snprintf(response, BUFSIZE, response_template, HTTP_METRICS_ENDPOINT);
  http_server.send(200, "text/plain; charset=utf-8", response);
}

void handle_http_not_found() {
    http_server.send(404, "text/plain; charset=utf-8", "Not found.");
}

void http_blink() {
  http_server.send(200, "text/plain; charset=utf-8", "Blinking!");
  esp8266_main_led(true);
  delay(200);
  esp8266_main_led(false);
}

void handle_http_metrics() {
  static bool has_sensed = false;
  static full_reading_t reading;
  static size_t const BUFSIZE = 2048;
  static char const *response_template =
      // exterior
      "# HELP " PROM_NAMESPACE "_exterior_humidity_percent Air humidity.\n"
      "# TYPE " PROM_NAMESPACE "_exterior_humidity_percent gauge\n"
      "# UNIT " PROM_NAMESPACE "_exterior_humidity_percent %%\n"
      PROM_NAMESPACE "_exterior_humidity_percent %f\n"
      "# HELP " PROM_NAMESPACE "_exterior_temperature_celsius Air temperature.\n"
      "# TYPE " PROM_NAMESPACE "_exterior_temperature_celsius gauge\n"
      "# UNIT " PROM_NAMESPACE "_exterior_temperature_celsius \u00B0C\n"
      PROM_NAMESPACE "_exterior_temperature_celsius %f\n"
      "# HELP " PROM_NAMESPACE "_exterior_heat_index_celsius Heat index.\n"
      "# TYPE " PROM_NAMESPACE "_exterior_heat_index_celsius gauge\n"
      "# UNIT " PROM_NAMESPACE "_exterior_heat_index_celsius \u00B0C\n"
      PROM_NAMESPACE "_exterior_heat_index_celsius %f\n"
      // interior 
      "# HELP " PROM_NAMESPACE "_interior_humidity_percent Air humidity.\n"
      "# TYPE " PROM_NAMESPACE "_interior_humidity_percent gauge\n"
      "# UNIT " PROM_NAMESPACE "_interior_humidity_percent %%\n"
      PROM_NAMESPACE "_interior_humidity_percent %f\n"
      "# HELP " PROM_NAMESPACE "_interior_temperature_celsius Air temperature.\n"
      "# TYPE " PROM_NAMESPACE "_interior_temperature_celsius gauge\n"
      "# UNIT " PROM_NAMESPACE "_interior_temperature_celsius \u00B0C\n"
      PROM_NAMESPACE "_interior_temperature_celsius %f\n"
      "# HELP " PROM_NAMESPACE "_interior_heat_index_celsius Heat index.\n"
      "# TYPE " PROM_NAMESPACE "_interior_heat_index_celsius gauge\n"
      "# UNIT " PROM_NAMESPACE "_interior_heat_index_celsius \u00B0C\n"
      PROM_NAMESPACE "_interior_heat_index_celsius %f\n";

  esp8266_main_led(true);
  if (!has_sensed || millis() - reading.millis > SENSE_EVERY) {
    reading = sense_all();
    if (!reading.success) {
        http_server.send(500, "text/plain; charset=utf-8", "Sensor error.");
        return;
    }

    has_sensed = true;
  }

  char response[BUFSIZE];
  snprintf(
    response,
    BUFSIZE,
    response_template,
    reading.exterior.humidity,
    reading.exterior.temp,
    reading.exterior.heat_index,
    reading.interior.humidity,
    reading.interior.temp,
    reading.interior.heat_index
  );

  http_server.send(200, "text/plain; charset=utf-8", response);

  Serial.println("Sent metrics");
  esp8266_main_led(false);
}

void setup_http_server() {
    Serial.println("Setting up HTTP server");
    http_server.on("/", HTTPMethod::HTTP_GET, handle_http_root);
    http_server.on(HTTP_METRICS_ENDPOINT, HTTPMethod::HTTP_GET, handle_http_metrics);
    http_server.on(HTTP_BLINK_ENDPOINT, HTTPMethod::HTTP_GET, http_blink);
    http_server.onNotFound(handle_http_not_found);
    http_server.begin();
    Serial.println("HTTP server started");
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
  esp8266_main_led(true);
  delay(500);
  esp8266_main_led(false);
}

// Main

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println("Starting up...");

    dht_int.begin();
    dht_ext.begin();

    setup_wifi();
    setup_http_server();
}

void loop() {
  http_server.handleClient();
}