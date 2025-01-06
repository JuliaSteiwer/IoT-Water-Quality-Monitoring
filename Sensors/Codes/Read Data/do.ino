// Julia Steiwer, 29. Dec. 2024
// Code for reading out values from the SEN0237-A dissolved oxygen (DO) sensor by DFRobot. Temperature correction is included.
// Please make sure to replace the voltage and temperature values in the high- and low-point calibration section (lines 22-32) with the values you measured using "calibrate_do.ino" — otherwise false values will be shown.

// Libraries
#include <Arduino.h>
// -- Temperature Libs
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Dissolved Oxygen (DO)
#define DO_PIN 3

#define VREF 3300    //VREF (mv)
#define ADC_RES 4096 //ADC Resolution

// Single-point calibration Mode=0
// Two-point calibration Mode=1
#define TWO_POINT_CALIBRATION 1

// Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (1600) // mV, get this with "calibrate_do.ino"
#define CAL1_T (25)   // ℃, higher temperature
// Two-point calibration needs to be filled CAL2_V and CAL2_T
// CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1300) // mV, get this with "calibrate_do.ino"
#define CAL2_T (15)   // ℃, lower temperature

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

uint8_t Temperaturet;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
uint16_t DO;

int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c)
{
#if TWO_POINT_CALIBRATION == 0
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}

void setup()
{
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Getting DO values: ");
}

void loop()
{
  
  // SENSOR INPUTS
  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it
  
  // Dissolved Oxygen
  Temperaturet = tempC;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  int do_raw = readDO(ADC_Voltage, Temperaturet);
  float DO = do_raw / 1000.0; // convert DO unit to mg/L

  Serial.print("Temperature [°C]:\t" + String(Temperaturet) + "\t");
  //Serial.print("ADC RAW:\t" + String(ADC_Raw) + "\t"); // not really needed but nice for debugging
  //Serial.print("ADC Voltage:\t" + String(ADC_Voltage) + "\t"); // not really needed but nice for debugging
  Serial.println("DO [mg/L]:\t" + String(DO) + "\t");

  delay(1000);
}

// Julia Steiwer, 29. Dec. 2024