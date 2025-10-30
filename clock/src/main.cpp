#include <Arduino.h>
#include "clock.hpp"
#include "defines.hpp"
#include "private.h"

const char* TZ_STRING = "EST5EDT,M3.2.0/2,M11.1.0/2";

clock_display disp;

const float min_brightness = 0.0001f;
const float max_brightness = 0.4f;

// functions

float lerp(float val, float in_min, float in_max, float out_min, float out_max) {
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void set_brightness(float frac) {
  frac = constrain(frac, 0.0f, 1.0f);
  int duty = (int)round((1.0f - frac) * 1023); // active-low OE
  analogWrite(OUTPUT_ENABLE, duty);
}

float read_light_level() {
  static float avg = 0.0f;
  static float alpha = 0.1f;
  static bool first = true;

  int raw = analogRead(A0);

  float level = constrain(raw / 1024.0f, 0.0f, 1.0f);
  if (first) {
    avg = level;
    first = false;
  } else {
    avg = avg * (1 - alpha) + level * alpha;
  }

  return avg;
}

void setup() {
  Serial.begin(9600);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(OUTPUT_ENABLE, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(SRCLR, OUTPUT);

  analogWriteFreq(8000);      // 8 kHz is a nice start (1–20 kHz typical)
  analogWriteRange(1023);     // default, included for clarity

  set_brightness(0.5f);
  disp.reset();

  // flash to indicate startup
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    disp.flash(0);
  }

  configTime(TZ_STRING, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for time...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    disp.flash(8);
  }

  Serial.println("Got time!");
}

void loop() {
  disp.do_second();

  float light_level = read_light_level();
  set_brightness(lerp(light_level, 0.0f, 1.0f, min_brightness, max_brightness));

  // delay the amount we need to for brightness updates, seconds will update on
  // their own internal schedule
  delay(50);
}