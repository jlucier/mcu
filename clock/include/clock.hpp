#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "defines.hpp"

#define NDIGITS 6
#define SE 0x01
#define SG 0x02
#define SF 0x04
#define SA 0x08
#define SB 0x10
#define SD 0x20
#define SC 0x40
#define SDP 0x80

using digit_t = uint8_t;

digit_t number_to_byte[] = {
    SA | SB | SC | SD | SE | SF,       // 0
    SB | SC,                           // 1
    SA | SB | SD | SE | SG,            // 2
    SA | SB | SC | SD | SG,            // 3
    SB | SC | SF | SG,                 // 4
    SA | SC | SD | SF | SG,            // 5
    SA | SC | SD | SE | SF | SG,       // 6
    SA | SB | SC,                      // 7
    SA | SB | SC | SD | SE | SF | SG,  // 8
    SA | SB | SC | SD | SF | SG,       // 9
};

class clock_display {
  tm timeinfo;
  digit_t digits[NDIGITS];
  bool colons = false;
  bool in_second = false;
  uint wait_ms = 500;
  uint last_update = 0;

  void write_all() {
    reset();

    // output register latch low
    digitalWrite(RCLK, LOW);

    for (int i = 0; i < NDIGITS; i++) {
      // shift out the data
      // TODO: place colons properly
      uint8_t out = this->digits[i] | (this->colons ? SDP : 0);
      shiftOut(DATA_PIN, SRCLK, MSBFIRST, out);
    }

    // latch high into the output registers
    digitalWrite(RCLK, HIGH);
  }

  void display_with_flash(uint delay_ms = 1000) {
    this->colons = true;
    write_all();
    delay(delay_ms / 2);
    this->colons = false;
    write_all();
    delay(delay_ms - delay_ms / 2);
  }

  uint display_second() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    uint ms_sleep = 1000 - (tv.tv_usec / 1000);
    ms_sleep = ms_sleep > 1000 || ms_sleep < 0 ? 1000 : ms_sleep;

    this->digits[0] = number_to_byte[timeinfo.tm_sec % 10];
    this->digits[1] = number_to_byte[timeinfo.tm_sec / 10];
    this->digits[2] = number_to_byte[timeinfo.tm_min % 10];
    this->digits[3] = number_to_byte[timeinfo.tm_min / 10];
    this->digits[4] = number_to_byte[timeinfo.tm_hour % 10];
    this->digits[5] = number_to_byte[timeinfo.tm_hour / 10];

    this->colons = true;
    write_all();
    return ms_sleep;
  }

  void colons_off() {
    this->colons = false;
    write_all();
  }


public:

  void reset() {
    digitalWrite(SRCLR, LOW);
    delay(10);
    digitalWrite(SRCLR, HIGH);
  }

  void flash(digit_t v) {
    digit_t low = v % 10;
    digit_t high = v / 10;

    for (int i = 0; i < NDIGITS; i++) {
      this->digits[i] = number_to_byte[i%2==0 ? low : high];
    }

    display_with_flash();
  }

  /**
   * Display the time, self-throttiling update frequency
   */
  void do_second() {
    static char buf[64];

    // make throttle updates
    uint now = millis();
    if ((now - this->last_update) < this->wait_ms) {
        return;
    }

    this->last_update = millis();

    if (in_second) {
        // toggle colons, draw, and wait again
        this->colons_off();
        this->in_second = false;
        return;
    }

    // update time, write second
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to get time");
        this->wait_ms = 500;
        return;
    }

    strftime(buf, sizeof(buf), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    Serial.println(buf);
    // do first half of second, setting wait to half the second
    this->wait_ms = display_second() / 2;
    this->in_second = true;
  }
};
