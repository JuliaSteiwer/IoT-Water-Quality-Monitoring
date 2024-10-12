// Calibration code for DFRobot pH Sensor SEN:SKU0161.
// Using a Heltec WiFi LoRa 32 (V3). -> based on ESP32
// The sensor is powered with the 5V and GND from the Heltec Board.
// As the GPIO pin of the board gets 3.3V and analogRead() = 0 to 4095, we have to multiply the analog input Value with 3.3 / 4096

// temperature libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Enable Pin
#define VReg 47

// TEMPERATURE SENSOR
#define ONE_WIRE_BUS 7         // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// PH SENSOR
#define T 273.15               // degrees Kelvin → for temperature correction
#define Alpha 0.05916          // alpha → for temperature correction
#define Offset 0.00 
#define LED 13
const int phPin = 6;           // reading the analog value via pin GPIO6
float ph;
float Value = 0;
float pHvoltage, pHcorr;       // define params for temperature-based pH correction

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(VReg, OUTPUT);
  digitalWrite(VReg, HIGH);   // turn the LED on (HIGH is the voltage level)

  Serial.println("Temperature experiment!");
}

void loop() {
  // put your main code here, to run repeatedly:
  // TEMPERATURE
  sensors.begin();                           // start up the sensor library
  sensors.requestTemperatures();             // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0);  // there's only one temperature sensor so we can index to it

  // PH VALUE
  pinMode(phPin,INPUT);                      // get the pH output
  static float pHValue,voltage;
  Value = analogRead(phPin);
  voltage = Value * (3.3/4096.0);            // ESP32 analogRead output is 0 to 4095, so divide by 4096 and pin receives 3.3V max
  pHValue = 3.5*voltage + Offset;
  pHvoltage = voltage / 327.68;
  pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage; 

  Serial.print("Temperature (°C): ");
  Serial.print(tempC);
  Serial.print(" | pH Voltage: ");
  Serial.print(voltage);
  Serial.print(" | pH Value: ");
  Serial.print(pHValue);
  Serial.print(" | pH Value (corrected): ");
  Serial.println(pHcorr);

  delay(1000);
}
