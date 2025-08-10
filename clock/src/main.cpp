#include <Arduino.h>

#define DATA_PIN D8
#define OUTPUT_ENABLE D7
#define RCLK D6
#define SRCLK D5
#define SRCLR D0

void reset() {
  digitalWrite(SRCLR, LOW);
  delay(10);
  digitalWrite(SRCLR, HIGH);
}

void output_enable() {
  digitalWrite(OUTPUT_ENABLE, LOW);
}

void setup() {
  pinMode(DATA_PIN, OUTPUT);
  pinMode(OUTPUT_ENABLE, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(SRCLR, OUTPUT);

  output_enable();
  reset();
}

void loop() {
  for (int numberToDisplay = 0; numberToDisplay < 256; numberToDisplay++) {
    digitalWrite(RCLK, LOW);

    // shift out the bits:
    shiftOut(DATA_PIN, SRCLK, MSBFIRST, numberToDisplay);

    //take the latch pin high so the LEDs will light up:
    digitalWrite(RCLK, HIGH);

    // pause before next value:
    delay(100);
  }
}