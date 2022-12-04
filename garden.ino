// #include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>

// #include "sensors.h"
#include "eth_server.h"

#define PROM_NAMESPACE "garden"
#define SENSE_EVERY 10000
#define HTTP_METRICS_ENDPOINT "/metrics"


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  // setup_sensors();
  EthHTTPServer::setup();
}

void loop() {
  // http_server.handleClient();
  // Serial.println("Hello!");
  // digitalWrite(LED_BUILTIN, HIGH);
  // delay(1000);
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(1000);

  EthHTTPServer::run();
}



// Setup / run

// void setup_http_server() {
//     Serial.println("Setting up HTTP server");
//     http_server.on("/", HTTPMethod::HTTP_GET, handle_http_root);
//     http_server.on(HTTP_METRICS_ENDPOINT, HTTPMethod::HTTP_GET, handle_http_metrics);
//     http_server.onNotFound(handle_http_not_found);
//     http_server.begin();
//     Serial.println("HTTP server started");
// }

// void setup_wifi() {
//   // // Begin WiFi
//   WiFi.begin(WIFI_SSID, WIFI_PASS);
 
//   // Connecting to WiFi...
//   Serial.print("Connecting to ");
//   Serial.print(WIFI_SSID);
//   // Loop continuously while WiFi is not connected
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(100);
//     Serial.print(".");
//   }
 
//   // Connected to WiFi
//   Serial.println();
//   Serial.print("Connected! IP address: ");
//   Serial.println(WiFi.localIP());
// }

EthHTTPServer::http_response handle_http_root() {
  static char const *response_template =
    "Prometheus ESP8266 Sensor Exporter.\n"
    "\n"
    "Referenced heavily from: https://github.com/HON95/prometheus-esp8266-dht-exporter\n"
    "\n"
    "Usage: %s\n";
  EthHTTPServer::http_response response;
  snprintf(response.body, 256, response_template, HTTP_METRICS_ENDPOINT);
  return response;
}

EthHTTPServer::http_response handle_http_not_found() {
  return EthHTTPServer::http_response {
    404,
    "text/plain; charset=utf-8",
    "Not found."    
  };
}

EthHTTPServer::http_response handle_http_metrics() {
  static bool has_sensed = false;
  // static sensor_reading_t reading;
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

  EthHTTPServer::http_response response;

  // if (!has_sensed || millis() - reading.millis > 5000) {
  //   reading = read_sensors();
  //   if (!reading.dht1.success || !reading.dht2.success) {
  //     response.code = 500;
  //     sprintf(response.data, "Sensing error!");
  //     return response;
  //   }

  //   has_sensed = true;
  // }

  snprintf(
    response.body,
    MAX_RESPONSE_LEN,
    response_template
    // reading.dht1.humidity,
    // reading.dht1.temp,
    // reading.dht2.humidity,
    // reading.dht2.temp,
    // reading.ph,
    // reading.liquid_temp
  );
  Serial.println("Sent metrics");
  return response;
}