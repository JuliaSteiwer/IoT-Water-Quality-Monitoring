// temperature libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Enable Pin
#define VReg 47

// TEMPERATURE SENSOR
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// PH SENSOR
#define T 273.15               // degrees Kelvin → for temperature correction
#define Alpha 0.05916          // alpha → for temperature correction
#define Offset -2.25 
#define LED 13
const int phPin = 6; // reading the analog value via pin GPIO6
float ph;
float Value = 0;
float pHvoltage, pHcorr; // define params for temperature-based pH correction

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
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // PH VALUE
  pinMode(phPin,INPUT); // get the pH output
  static float pHValue,voltage;
  Value = analogRead(phPin);
  voltage = Value * (5.0/4096.0); // ESP32 analogRead output is 0 to 4095, so divide by 4096
  pHValue = 3.5*voltage + Offset;
  pHvoltage = voltage / 327.68;
  pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage; 

  Serial.print("Temperatur (°C): ");
  Serial.print(tempC);
  Serial.print("Analog: ");
  Serial.print(Value);
  Serial.print(" | Spannung pH: ");
  Serial.print(voltage);
  Serial.print(" | pH Wert: ");
  Serial.print(pHValue);
  Serial.print(" | pH Wert (korr): ");
  Serial.println(pHcorr);

  delay(1000);
}
