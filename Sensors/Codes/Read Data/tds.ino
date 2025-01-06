// Julia Steiwer, 29. Dec. 2024
// Read TDS value from SKU:SEN0244 sensor by DFRobot with ESP32. Temperature correction is included.

// Libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// -- total dissolved solids (TDS)
#define tdsPin 5
int tdsSensorValue = 0;
float tdsValue = 0;
float tdsVoltage = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // baud rate for ESP32
  delay(2000); 
  Serial.println("Measuring TDS: ");
}

void loop() {
  // put your main code here, to run repeatedly:

  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // -- TDS (total dissolved solids)
  // #include "GravityTDS.h" only works with Arduino UNO and not with ESP32; since I was too lazy to rewrite the library so I put everything here.
  tdsSensorValue = analogRead(tdsPin);
  tdsVoltage = tdsSensorValue*(3.3/4096.0);
  float compensationCoefficient=1.0+0.02*(tempC-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compVoltage=tdsVoltage/compensationCoefficient;
  tdsValue=(133.42/compVoltage*compVoltage*compVoltage - 255.86*compVoltage*compVoltage + 857.39*compVoltage)*0.5;

  // Print out the results:
  Serial.print("Temperature [°C]: ");
  Serial.print(tempC);
  Serial.print(" | TDS [ppm]: ");
  Serial.println(tdsValue);
}

// Julia Steiwer, 29. Dec. 2024
