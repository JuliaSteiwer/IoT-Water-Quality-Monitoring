// Calibrate DFRobot pH sensor SKU:SEN0161 with Arduino UNO
// Temperature compensation with DS18B20 temperature sensor.

#include "Arduino.h"
#include <OneWire.h>
#include <DallasTemperature.h> // DS18B20 library

// -- Temperature
#define ONE_WIRE_BUS A1         // data wire is plugged to A1
#define T 273.15                // degrees Kelvin
#define Alpha 0.05916           // alpha

#define phPin A0                //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.00             //deviation compensate
#define LED 13
#define T 273.15                // degrees Kelvin
#define Alpha 0.05916           // alpha

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

float Value = 0;
float pHcorr, pHvoltage; // corrected pH value

void setup() {
  // put your setup code here, to run once:
  pinMode(LED,OUTPUT);
  Serial.begin(9600);
  Serial.println("pH meter experiment!");
}

void loop() {
  // put your main code here, to run repeatedly:
  // TEMPERATURE
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  pinMode(phPin,INPUT); // get the pH output
  static float pHValue,voltage;

  Value = analogRead(phPin);
  voltage = Value * (5.0 / 1024.0);
  pHValue = 3.5 * voltage + Offset; // + Offset if there is any.
  pHvoltage = voltage / 327.68;
  pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage; // temperature-corrected pH value

  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.print(" | Voltage pH: ");
  Serial.print(voltage);
  Serial.print(" | pH Value: ");
  Serial.print(pHValue);
  Serial.print(" | pH Value (corrected): ");
  Serial.println(pHcorr);

  delay(1000);
}
