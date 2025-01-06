// Julia Steiwer, 29. Dec. 2024
// Fallback for reading out the EC sensor (DFR0300 by DFRobot) in case the EC library by DFRobot or GreenPonik (linked in ec.ino) do not work.

// Make sure to calibrate the sensor first, using "calibrate_ec.ino" — if the library in that code works properly, you can use the code for both calibration (by entering the commands mentioned at the top) and for reading out sensor values.

// SENSOR LIBRARIES
// temperature libraries
#include <OneWire.h>
#include <DallasTemperature.h>
// -- electrical conductivity (EC)
#include <EEPROM.h>

// SENSOR DEFINITIONS
// -- Temperature
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// -- electrical conductivity (EC)
#define EC_PIN 4
#define RES2 820.0        // grabbed from lib because integration did not work
#define ECREF 200.0       // same as above
float ecVoltage,ecValue;
float kvalue = 1.00;      // kvalue which you get from calibration (should be almost 1.00 when new)

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Getting EC values: ");
}

void loop() {
  // put your main code here, to run repeatedly:

  // SENSOR INPUTS
  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // -- electrical conductivity (EC)
  // Most of this code is only here because I faced issues with the EC library. If you do not face issues with it, just use "ec.h" instead.
  ecVoltage = analogRead(EC_PIN)/4096.0*3300; // 1024.0*5000 for Arduino, 4096*3300 for ESP32
  float rawEC = 1000*ecVoltage/RES2/ECREF;
  float valueTemp = rawEC * kvalue; // 1.0 is the k-value (K = 1)
  if(valueTemp > 2.5){
    kvalue = 1.00; // kvalue which you get from calibration (should be almost 1.00 when new)
  }else if(valueTemp < 2.0){
    kvalue = 1.00; // kvalue which you get from calibration (should be almost 1.00 when new)
  }
  float value = rawEC * kvalue;             //calculate the EC value after automatic shift
  value = value / (1.0+0.0185*(tempC-25.0));  //temperature compensation
  float ecValue = value;

  // Print out the results:
  Serial.print("Temperature [°C]: ");
  Serial.print(tempC);
  Serial.print(" | EC [mS/cm]: ");
  Serial.println(ecValue);

}

// Julia Steiwer, 29. Dec. 2024
