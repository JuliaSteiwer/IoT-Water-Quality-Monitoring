// Julia Steiwer, 29. Dec. 2024
// Use EC sensor DFR0300 sensor by DFRobot using ESP32. Temperature correction is included.

/*
 * file DFRobot_EC.ino
 * @ https://github.com/DFRobot/DFRobot_EC
 *
 * This is the sample code for Gravity: Analog Electrical Conductivity Sensor / Meter Kit V2 (K=1.0), SKU: DFR0300.
 * In order to guarantee precision, a temperature sensor such as DS18B20 is needed, to execute automatic temperature compensation.
 * You can send commands in the serial monitor to execute the calibration.
 * Serial Commands:
 *   enterec -> enter the calibration mode
 *   calec -> calibrate with the standard buffer solution, two buffer solutions(1413us/cm and 12.88ms/cm) will be automaticlly recognized
 *   exitec -> save the calibrated parameters and exit from calibration mode
 *
 * Copyright   [DFRobot](http://www.dfrobot.com), 2018
 * Copyright   GNU Lesser General Public License
 *
 * version  V1.0
 * date  2018-03-21
 */

// ADDITIONAL NOTE: It might be required to make changes to the DFRobot_EC library (https://github.com/DFRobot/DFRobot_EC) as it is optimized for Arduino UNO and not for ESP32. You could also try using https://github.com/GreenPonik/DFRobot_ESP_EC_BY_GREENPONIK as an alternative. The below code however has been rewritten for ESP32 compatibility.
// Use the above commands for calibration. If you don't use them and run the code, you should be able to just read out the sensor values.

// Libraries
// -- temperature
#include <OneWire.h>
#include <DallasTemperature.h>
// -- electrical conductivity (EC)
#include "DFRobot_EC.h" // either rewrite https://github.com/DFRobot/DFRobot_EC for compatibility with ESP32 or try using https://github.com/GreenPonik/DFRobot_ESP_EC_BY_GREENPONIK instead
#include <EEPROM.h>

// Temperature
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// EC
#define EC_PIN 4
float voltage,ecValue;
DFRobot_EC ec;

void setup()
{
  Serial.begin(115200);  
  ec.begin();
}

void loop()
{
    static unsigned long timepoint = millis();
    if(millis()-timepoint>1000U)  //time interval: 1s
    {
      timepoint = millis();
      voltage = analogRead(EC_PIN)/4096.0*3300;   // read the voltage
      
      // -- DS18B20 Temperature
      // We're reading the temperature from this sensor to do temperature correction on the EC sensor.
      sensors.begin(); // start up the sensor library
      sensors.requestTemperatures(); // Send the command to get temperatures
      float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

      ecValue =  ec.readEC(voltage,tempC);  // convert voltage to EC with temperature compensation
      Serial.print("temperature:");
      Serial.print(tempC,1);
      Serial.print("^C  EC:");
      Serial.print(ecValue,2);
      Serial.println("ms/cm");
    }
    ec.calibration(voltage,tempC);          // calibration process by Serail CMD
}

// Julia Steiwer, 29. Dec. 2024