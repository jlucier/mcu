#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "private.h"

#define DATA_PIN D8
#define OUTPUT_ENABLE D7
#define RCLK D6
#define SRCLK D5
#define SRCLR D0

using digit_t = uint8_t;
#define NDIGITS 6

#define SE 0x01
#define SG 0x02
#define SF 0x04
#define SA 0x08
#define SB 0x10
#define SD 0x20
#define SC 0x40
#define SDP 0x80

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

const char* TZ_STRING = "EST5EDT,M3.2.0/2,M11.1.0/2";

struct clock_display {
  digit_t digits[NDIGITS];
  bool colons = false;
};

clock_display disp;

// functions

void reset() {
  digitalWrite(SRCLR, LOW);
  delay(10);
  digitalWrite(SRCLR, HIGH);
}

void output_enable() {
  digitalWrite(OUTPUT_ENABLE, LOW);
}

void write_all(clock_display& disp) {
  reset();

  // output register latch low
  digitalWrite(RCLK, LOW);

  for (int i = 0; i < NDIGITS; i++) {
    // shift out the data
    // TODO: place colons properly
    uint8_t out = disp.digits[i] | (disp.colons ? SDP : 0);
    shiftOut(DATA_PIN, SRCLK, MSBFIRST, out);
  }

  // latch high into the output registers
  digitalWrite(RCLK, HIGH);
}

void display_with_flash(clock_display& disp, uint64_t delay_ms = 1000) {
  disp.colons = true;
  write_all(disp);
  delay(delay_ms / 2);
  disp.colons = false;
  write_all(disp);
  delay(delay_ms - delay_ms / 2);
}

void second(clock_display& disp, tm& timeinfo) {
  timeval tv;
  gettimeofday(&tv, nullptr);
  uint64_t ms_sleep = 1000 - (tv.tv_usec / 1000);
  ms_sleep = ms_sleep > 1000 || ms_sleep < 0 ? 1000 : ms_sleep;

  disp.digits[0] = number_to_byte[timeinfo.tm_sec % 10];
  disp.digits[1] = number_to_byte[timeinfo.tm_sec / 10];
  disp.digits[2] = number_to_byte[timeinfo.tm_min % 10];
  disp.digits[3] = number_to_byte[timeinfo.tm_min / 10];
  disp.digits[4] = number_to_byte[timeinfo.tm_hour % 10];
  disp.digits[5] = number_to_byte[timeinfo.tm_hour / 10];

  display_with_flash(disp, ms_sleep);
}

void flash(clock_display& disp, int i) {
  int low = i % 10;
  int high = i / 10;

  disp.digits[0] = number_to_byte[low];
  disp.digits[1] = number_to_byte[high];
  disp.digits[2] = number_to_byte[low];
  disp.digits[3] = number_to_byte[high];
  disp.digits[4] = number_to_byte[low];
  disp.digits[5] = number_to_byte[high];

  display_with_flash(disp);
}

void setup() {
  Serial.begin(9600);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(OUTPUT_ENABLE, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(SRCLR, OUTPUT);

  output_enable();
  reset();

  // flash to indicate startup
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    flash(disp, 0);
  }

  configTime(TZ_STRING, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for time...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    flash(disp, 8);
  }

  Serial.println("Got time!");
}

void loop() {
  static char buf[64];
  static struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {

    strftime(buf, sizeof(buf), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    Serial.println(buf);

    second(disp, timeinfo);
  } else {
    Serial.println("Failed to get time");
    delay(500);
  }
}