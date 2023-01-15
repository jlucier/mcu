// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include "pico-onewire/api/one_wire.h"
#include "pico-onewire/source/one_wire.cpp"


#define DHT1_PIN 13
#define DHT2_PIN 14
#define ONE_WIRE_BUS 15
#define PH_PIN A2
#define SENSOR_SLOPE -0.00563

DHT_Unified dht1(DHT1_PIN, DHT22);
DHT_Unified dht2(DHT2_PIN, DHT22);
One_wire one_wire(ONE_WIRE_BUS);

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

float read_ph(bool print) {
    int raw = analogRead(PH_PIN);
    float mv = raw / 4096.0 * 3300;  // read milivolts
    float ph = raw * -0.01802851203 + 15.94402376;

    if (print) {
        Serial.print("Raw read: ");
        Serial.println(raw);
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
    one_wire.init();
}

float read_liquid_temp() {
    rom_address_t address{};
    one_wire.single_device_read_rom(address);
    one_wire.convert_temperature(address, true, false);
    return one_wire.temperature(address);
}


// setup

void setup_sensors() {
    setup_dht();
    setup_temp();
}

sensor_reading_t read_sensors(bool print=false) {
    return sensor_reading_t {
        read_dht(&dht1),
        read_dht(&dht2),
        read_ph(true),
        read_liquid_temp(),
        millis(),
    };
}

// tools

void calibrate_ph() {
    delay(1000);
    sensor_reading_t reading = read_sensors();
    Serial.print("temp 1: ");
    Serial.print(reading.dht1.temp);
    Serial.print("   temp 2: ");
    Serial.println(reading.dht2.temp);
    Serial.println();
}
