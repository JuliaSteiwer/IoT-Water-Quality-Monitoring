// Julia Steiwer, 29. Dec. 2024
// Read temperature from DS18B20 sensor with ESP32.

// Libraries
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200); // baud rate for ESP32
  delay(2000); 
  Serial.println("Measuring temperature");
}

void loop() {
  // put your main code here, to run repeatedly:

  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it
  
  // Print temperature info to serial monitor
  Serial.print("Temperature [°C] = ");
  Serial.println(tempC);

  delay(1000);
}

// Julia Steiwer, 29. Dec. 2024
