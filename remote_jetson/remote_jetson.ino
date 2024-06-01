#define SERVER_PORT 80
#include "utils/eth_server.h"

#define POWER_PIN 22
#define REC_PIN 19
#define POWER_BTN 26

struct pin_state_t {
    bool power = false;
    bool recovery = false;
};

static pin_state_t pin_state;

void setup() {
    Serial.begin(115200);
    // while (!Serial) delay(10);

    // pins
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(POWER_PIN, OUTPUT);
    pinMode(REC_PIN, OUTPUT);
    pinMode(POWER_BTN, OUTPUT);

    // server
    setup_server();
}

void loop() {
    EthHTTPServer::run();
}

const char* bool_to_str(bool b) {
  return b ? "true" : "false";
}

EthHTTPServer::http_response make_state_response() {
    static const char* state_template =
        "{\n"
        "  \"power\": %s,\n"
        "  \"recovery\": %s\n"
        "}\n";
    EthHTTPServer::http_response response {
      .content_type = "application/json; charset=utf-8"
    };

    snprintf(
        response.body,
        256,
        state_template,
        bool_to_str(pin_state.power),
        bool_to_str(pin_state.recovery)
    );
    return response;
}

void set_pin(int pin_num, bool& pin, bool val) {
    digitalWrite(pin_num, val ? HIGH : LOW);
    pin = val;
}

void blink() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
}

EthHTTPServer::http_response blink_and_respond() {
    blink();
    return make_state_response();
}

// Setup / run

void setup_server() {
    EthHTTPServer::setup(EthHTTPServer::EthServerConfig{
      .pin_rx = 0,
      .pin_tx = 3,
      .pin_cs = 1,
      .pin_sck = 2,
    });
    EthHTTPServer::add_endpoint("/", &handle_http_root);
    EthHTTPServer::add_endpoint("/state", &http_state);
    EthHTTPServer::add_endpoint("/power/on", &http_power_on);
    EthHTTPServer::add_endpoint("/power/off", &http_power_off);
    EthHTTPServer::add_endpoint("/recovery/on", &http_recovery_on);
    EthHTTPServer::add_endpoint("/recovery/off", &http_recovery_off);
    EthHTTPServer::add_endpoint("/press_power_btn", &http_press_power_btn);
}

EthHTTPServer::http_response handle_http_root(const EthHTTPServer::http_request&) {
    static const char* msg = "Jetson remote control.\n";
    EthHTTPServer::http_response response;
    snprintf(response.body, 256, msg);
    return response;
}

EthHTTPServer::http_response http_state(const EthHTTPServer::http_request&) {
    return blink_and_respond();
}

EthHTTPServer::http_response http_power_on(const EthHTTPServer::http_request&) {
    set_pin(POWER_PIN, pin_state.power, true);
    return blink_and_respond();
}

EthHTTPServer::http_response http_power_off(const EthHTTPServer::http_request&) {
    set_pin(POWER_PIN, pin_state.power, false);
    return blink_and_respond();
}

EthHTTPServer::http_response http_press_power_btn(const EthHTTPServer::http_request&) {
    digitalWrite(POWER_BTN, HIGH);
    delay(350);
    digitalWrite(POWER_BTN, LOW);
    return blink_and_respond();
}

 EthHTTPServer::http_response http_recovery_on(const EthHTTPServer::http_request&) {
     set_pin(REC_PIN, pin_state.recovery, true);
     return blink_and_respond();
 }

 EthHTTPServer::http_response http_recovery_off(const EthHTTPServer::http_request&) {
     set_pin(REC_PIN, pin_state.recovery, false);
     return blink_and_respond();
}
