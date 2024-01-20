// Code provided by DFRobot.
// Calibrate the dissolved oxygen (DO) sensor.
// Sensor: SEN0237
// This sensor has an analog ouput.
// For more information on calibrating the sensor, please head here:
// https://wiki.dfrobot.com/Gravity__Analog_Dissolved_Oxygen_Sensor_SKU_SEN0237

#include <Arduino.h>

#define VREF    5000//VREF(mv) // change to 3300 if your board has 3.3V power supply for sensors
#define ADC_RES 1024//ADC Resolution // you'll have to adapt this then, too

uint32_t raw;

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    raw=analogRead(A1); // make sure to put the right pin here!
    Serial.println("raw:\t"+String(raw)+"\tVoltage(mv)"+String(raw*VREF/ADC_RES));
    delay(1000);
}
