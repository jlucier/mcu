#include <Arduino.h>

#define DATA_PIN D8
#define OUTPUT_ENABLE D7
#define RCLK D6
#define SRCLK D5
#define SRCLR D0

using digit_t = uint8_t;
#define NDIGITS 1

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

struct clock_display {
  digit_t digits[1];
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

  // TODO: place colons properly
  uint8_t out = disp.digits[0] | (disp.colons ? SDP : 0);

  // for (int i = 0; i < NDIGITS; i++) {
  shiftOut(DATA_PIN, SRCLK, MSBFIRST, out);
  // }

  // latch high into the output registers
  digitalWrite(RCLK, HIGH);
}

void second(clock_display& disp, uint8_t i) {
  disp.digits[0] = number_to_byte[i];
  disp.colons = true;
  write_all(disp);
  delay(500);
  disp.colons = false;
  write_all(disp);
  delay(500);
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
}

void loop() {
  Serial.println("LOOOOOOOP");
  // disp.colons = !disp.colons;
  for (int i = 0; i < 10; i++) {
    second(disp, i);
  }
}