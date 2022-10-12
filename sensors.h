// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define DHT1_PIN 13
#define DHT2_PIN 12
#define ONE_WIRE_BUS 2
#define PH_PIN A0
#define SENSOR_SLOPE -0.00563

DHT_Unified dht1(DHT1_PIN, DHT22);
DHT_Unified dht2(DHT2_PIN, DHT22);
OneWire one_wire(ONE_WIRE_BUS);
DallasTemperature temp_sensors(&one_wire);

struct dht_reading_t {
  float temp; // c
  float humidity;
  bool success;
};

struct sensor_reading_t {
  dht_reading_t dht1;
  dht_reading_t dht2;
  float ph;
  float liquid_temp;
  uint millis;
};

// PH

void setup_ph() {
}

float read_ph(bool print) {

  delay(2000);
  float mv = analogRead(PH_PIN) / 1024.0 * 3300;  // read milivolts
  float ph = SENSOR_SLOPE * (mv - 1500) + 7;

  if (print) {
    Serial.print("Analog V: ");
    Serial.println(mv);
    Serial.print("PH: ");
    Serial.println(ph, 3);
  }

  return ph;
}


// DHTs

void setup_dht() {
  dht1.begin();
  dht2.begin();
  sensor_t sensor;
  dht1.temperature().getSensor(&sensor);
}

dht_reading_t read_dht(DHT_Unified* dht) {
  // Get temperature event and print its value.
  sensors_event_t event;
  dht_reading_t result;
  result.success = true;

  dht->temperature().getEvent(&event);
  result.temp = event.temperature;
  result.success &= !isnan(event.temperature);
  dht->humidity().getEvent(&event);
  result.humidity = event.relative_humidity;
  result.success &= !isnan(event.relative_humidity);
  return result;
}

void print_reading(dht_reading_t* reading) {
  Serial.print("Temp: ");
  Serial.print(reading->temp);
  Serial.println(" F");
  Serial.print("Humidity: ");
  Serial.print(reading->humidity);
  Serial.println("%");
}


// temp

void setup_temp() {
  temp_sensors.begin();
}

float read_liquid_temp() {
  temp_sensors.requestTemperatures();
  return temp_sensors.getTempCByIndex(0);
}


// setup

void setup_sensors() {
  setup_ph();
  setup_dht();
  setup_temp();
}

sensor_reading_t read_sensors() {
  return sensor_reading_t {
    read_dht(&dht1),
    read_dht(&dht2),
    read_ph(false),
    read_liquid_temp(),
    millis(),
  };
}