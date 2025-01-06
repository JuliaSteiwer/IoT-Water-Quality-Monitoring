// Julia Steiwer, 29. Dec. 2024
// Calibrate TDS sensor SKU:SEN0244 sensor by DFRobot using ESP32. Temperature correction is included.

/***************************************************
 DFRobot Gravity: Analog TDS Sensor/Meter
 <https://www.dfrobot.com/wiki/index.php/Gravity:_Analog_TDS_Sensor_/_Meter_For_Arduino_SKU:_SEN0244>

 ***************************************************
 This sample code shows how to read the tds value and calibrate it with the standard buffer solution.
 707ppm(1413us/cm)@25^c standard buffer solution is recommended.

 Created 2018-1-3
 By Jason <jason.ling@dfrobot.com@dfrobot.com>

 GNU Lesser General Public License.
 See <http://www.gnu.org/licenses/> for details.
 All above must be included in any redistribution.
 ****************************************************/

 /***********Notice and Trouble shooting***************
 1. This code is tested on Arduino Uno with Arduino IDE 1.0.5 r2 and 1.8.2.
 2. Calibration CMD:
     enter -> enter the calibration mode
     cal:tds value -> calibrate with the known tds value(25^c). e.g.cal:707
     exit -> save the parameters and exit the calibration mode
 ****************************************************/

// ADDITIONAL NOTE: It might be required to make changes to the GravityTDS library (https://github.com/DFRobot/GravityTDS) as it is optimized for Arduino UNO and not for ESP32. The below code however has been rewritten for ESP32 compatibility.
// This code is only for calibrating the TDS sensor (SKU:SEN0244 by DFRobot)!

// Libraries
// -- Temperature Libs
#include <OneWire.h>
#include <DallasTemperature.h>
// -- TDS Libs
#include <EEPROM.h>
#include "GravityTDS.h"

// Temperature
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// TDS
#define TdsSensorPin 5
GravityTDS gravityTds;
float tdsValue = 0;

void setup()
{
    Serial.begin(115200);
    delay(2000); 
  Serial.println("Calibrating TDS sensor: ");
    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(3.3);  //reference voltage on ADC, default 5.0V on Arduino UNO, changed to 3.3V for ESP32
    gravityTds.setAdcRange(4096);  //1024 for 10bit ADC;4096 for 12bit ADC, changed to 4096 for ESP32
    gravityTds.begin();  //initialization
}

void loop()
{
  // SENSOR INPUTS
  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // TDS Calibration
  gravityTds.setTemperature(tempC);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value
  Serial.print(tdsValue,0);
  Serial.println("ppm");
  delay(1000);
}

// Julia Steiwer, 29. Dec. 2024