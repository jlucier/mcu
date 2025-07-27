#include <Arduino.h>

#define ETH_DISABLE_HEADERS
#define ETH_DISABLE_BODY
#define ETH_PARSE_BUFFER_SIZE 128
#define ETH_MAX_ROUTES 2
#define ETH_MAX_ROUTE_TARGET 8

#include "utils/eth_server.h"


#define DT_PIN_PWR 7
#define DT_PIN_RST 8
#define DT_PIN_HOLD_DURATION 250

static const byte mac[] = {0xDE, 0xAD, 0xBE, 0xEE, 0xEE, 0xEF};

void toggle_pin(int pin) {
    digitalWrite(pin, HIGH);
    delay(DT_PIN_HOLD_DURATION);
    digitalWrite(pin, LOW);
}

EthHTTPServer::http_response handle_reset(const EthHTTPServer::http_request&) {
    toggle_pin(DT_PIN_RST);
    return EthHTTPServer::http_response{};
}

EthHTTPServer::http_response handle_power(const EthHTTPServer::http_request&) {
    toggle_pin(DT_PIN_PWR);
    return EthHTTPServer::http_response{};
}

void setup_server() {
    EthHTTPServer::EthServerConfig config;
    memcpy(config.mac, mac, sizeof(mac));

    EthHTTPServer::setup(config);
    EthHTTPServer::add_endpoint("/reset", &handle_reset);
    EthHTTPServer::add_endpoint("/power", &handle_power);
}

void setup() {
    Serial.begin(9600);
    Serial.println("Starting DT Remote Server...");
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(DT_PIN_RST, OUTPUT);
    pinMode(DT_PIN_PWR, OUTPUT);

    setup_server();
}

void loop() {
    EthHTTPServer::run();
}