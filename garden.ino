#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "private.h"
#include "sensors.h"

#define PROM_NAMESPACE "garden"
#define HTTP_METRICS_ENDPOINT "/metrics"
#define HTTP_SERVER_PORT 80
#define SENSE_EVERY 10000

ESP8266WebServer http_server(HTTP_SERVER_PORT);

void setup_server() {
  setup_wifi();
  setup_http_server();
}

void setup() {
  Serial.begin(115200);
  setup_sensors();
  setup_server();
}

void loop() {
  http_server.handleClient();
}

void calibrate_ph() {
  delay(1000);
  sensor_reading_t reading = read_sensors();
  Serial.print("temp 1: ");
  Serial.print(reading.dht1.temp);
  Serial.print("   temp 2: ");
  Serial.println(reading.dht2.temp);
  Serial.println();
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

void handle_http_metrics() {
  static bool has_sensed = false;
  static sensor_reading_t reading;
  static size_t const BUFSIZE = 2048;
  static char const *response_template =
      "# HELP " PROM_NAMESPACE "_air_humidity_percent Air humidity.\n"
      "# TYPE " PROM_NAMESPACE "_air_humidity_percent gauge\n"
      "# UNIT " PROM_NAMESPACE "_air_humidity_percent %%\n"
      PROM_NAMESPACE "_air_humidity_percent %f\n"
      "# HELP " PROM_NAMESPACE "_air_temperature_celsius Air temperature.\n"
      "# TYPE " PROM_NAMESPACE "_air_temperature_celsius gauge\n"
      "# UNIT " PROM_NAMESPACE "_air_temperature_celsius \u00B0C\n"
      PROM_NAMESPACE "_air_temperature_celsius %f\n"
      "# HELP " PROM_NAMESPACE "_bucket_humidity_percent Bucket humidity.\n"
      "# TYPE " PROM_NAMESPACE "_bucket_humidity_percent gauge\n"
      "# UNIT " PROM_NAMESPACE "_bucket_humidity_percent %%\n"
      PROM_NAMESPACE "_bucket_humidity_percent %f\n"
      "# HELP " PROM_NAMESPACE "_bucket_temperature_celsius Bucket temperature.\n"
      "# TYPE " PROM_NAMESPACE "_bucket_temperature_celsius gauge\n"
      "# UNIT " PROM_NAMESPACE "_bucket_temperature_celsius \u00B0C\n"
      PROM_NAMESPACE "_bucket_temperature_celsius %f\n"
      "# HELP " PROM_NAMESPACE "_solution_ph Solution ph.\n"
      "# TYPE " PROM_NAMESPACE "_solution_ph gauge\n"
      "# UNIT " PROM_NAMESPACE "_solution_ph pH\n"
      PROM_NAMESPACE "_solution_ph %f\n"
      "# HELP " PROM_NAMESPACE "_solution_temperature_celsius Solution temperature.\n"
      "# TYPE " PROM_NAMESPACE "_solution_temperature_celsius gauge\n"
      "# UNIT " PROM_NAMESPACE "_solution_temperature_celsius \u00B0C\n"
      PROM_NAMESPACE "_solution_temperature_celsius %f\n";

  if (!has_sensed || millis() - reading.millis > 5000) {
    reading = read_sensors();
    if (!reading.dht1.success || !reading.dht2.success) {
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
    reading.dht1.humidity,
    reading.dht1.temp,
    reading.dht2.humidity,
    reading.dht2.temp,
    reading.ph,
    reading.liquid_temp
  );
  http_server.send(200, "text/plain; charset=utf-8", response);
  Serial.println("Sent metrics");
}

// Setup / run

void setup_http_server() {
    Serial.println("Setting up HTTP server");
    http_server.on("/", HTTPMethod::HTTP_GET, handle_http_root);
    http_server.on(HTTP_METRICS_ENDPOINT, HTTPMethod::HTTP_GET, handle_http_metrics);
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
}