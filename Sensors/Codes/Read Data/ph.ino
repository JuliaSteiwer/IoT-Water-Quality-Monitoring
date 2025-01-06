// Julia Steiwer, 29. Dec. 2024
// Read pH value with SKU:SEN0161 sensor by DFRobot using ESP32. Temperature correction is included.

// CALIBRATION NOTES:
// Put the pH electrode into a standard solution with pH value = 7.00.
// In the serial monitor, you should now see the pH value, and for a new sensor (SKU:SEN0161 by DFRobot), the measurement error should not exceed 0.3. 
// Record the printed pH value,then compared with 7.00; the difference should be entered as the "Offset" value in the code.
// For example, if the pH value printed is 6.88, the difference is 0.12, so "# define Offset 0.00" should be changed to "# define Offset 0.12" in your program. 
// Put the pH electrode into a pH standard solution with pH = 4.00, then wait about one minute, adjust the gain potential device, and let the value stabilise at around 4.00
// Two-point calibration is sufficient. Solutions with high pH values are not very stable and can change their pH value upon contact with air.

// Libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// pH
#define T 273.15               // degrees Kelvin → for temperature correction
#define Alpha 0.05916          // alpha → for temperature correction
#define Offset 0.00            // offset has to be set depending on the calibration
#define LED 13
const int phPin = 6; // reading the analog value via pin GPIO6
float ph;
float Value = 0;
float pHvoltage, pHcorr; // define params for temperature-based pH correction

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // baud rate for ESP32
  delay(2000); 
  Serial.println("Measuring pH value");
}

void loop() {
  // put your main code here, to run repeatedly:

  // SENSOR INPUTS
  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // -- pH
  pinMode(phPin,INPUT); // get the pH output
  static float pHValue,voltage;
  Value = analogRead(phPin);
  voltage = Value * (3.3/4096.0); // ESP32 analogRead output is 0 to 4095, so divide by 4096; 3.3 because the GPIO only received a max of 3.3
  pHValue = 3.5*voltage;
  pHvoltage = voltage / 327.68;
  pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage; // pH corrected for temperature

  // Print temperature info to serial monitor
  Serial.print("Temperature [°C] = ");
  Serial.print(tempC);
  Serial.print(" | pH = ");
  Serial.println(pHcorr);

  delay(1000);
}

// Julia Steiwer, 29. Dec. 2024
