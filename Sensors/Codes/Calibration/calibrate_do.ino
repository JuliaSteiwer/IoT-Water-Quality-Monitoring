// Julia Steiwer, 29. Dec. 2024

// Calibration code for the dissolved oxygen (DO) sensor (SEN0237-A by DFRobot). Temperature correction is included.
// Reads the temperature of the liquid and the Voltage in mV corresponding to the oxygen content in the liquid.
// This information is required to set the high and low calibration points in the main code. So make sure you note down the °C and mV values you're seeing in the serial monitor for the two liquids.

// Libraries
#include <Arduino.h>
// -- Temperature Libs
#include <OneWire.h>
#include <DallasTemperature.h>

#define VREF    3300    // VREF(mv) of ESP32
#define ADC_RES 4096    // ADC Resolution of ESP32

// Temperature
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// DO
#define DO_PIN 3 // pin on ESP32 where we put the DO sensor

uint32_t raw;

void setup()
{
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Calibrating DO sensor: ");
}

void loop()
{
  // SENSOR INPUTS
  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it
  Serial.println("Temperature: " + String(tempC) + "°C"); // this way you also know the exact temperature for your calibration point
  
  // Get mV for oxygen content.
  raw=analogRead(DO_PIN);
  Serial.println("raw:\t"+String(raw)+"\tVoltage(mv)"+String(raw*VREF/ADC_RES));
  delay(1000);
}

// Julia Steiwer, 29. Dec. 2024