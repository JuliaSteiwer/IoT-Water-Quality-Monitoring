// Include the libraries
// -- temperature (T)
#include <OneWire.h>
#include <DallasTemperature.h>
// -- total dissolved solids (TDS)
#include <EEPROM.h>
#include "GravityTDS.h"
// -- electrical conductivity (EC)
#include "DFRobot_EC.h"
#include <EEPROM.h>
// -- dissolved oxygen (DO)
#include <Arduino.h>

// DEFINITIONS
// -- pH Value
#define SensorPin 14            //pH meter Analog output to Arduino Analog Input 0
#define LED 13
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40    //times of collection
// -- Temperature
#define ONE_WIRE_BUS 46         // data wire plugged to port 2
#define T 273.15               // degrees Kelvin
#define Alpha 0.05916          // alpha
// -- Total Dissolved Solids (TDS)
#define TdsSensorPin 15
GravityTDS gravityTds;
// -- Electrical Conductivity (EC)
#define EC_PIN 17
// -- Dissolved Oxygen (DO)
#define DO_PIN 18
#define VREF 5000    //VREF (mv)
#define ADC_RES 1024 //ADC Resolution
#define TWO_POINT_CALIBRATION 1
#define CAL1_V (1479) //mv
#define CAL1_T (29)   //℃
#define CAL2_V (834) //mv
#define CAL2_T (11)   //℃
// -- Turbidity
#define TU_PIN 16

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// VARIABLES
//-- ph
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;
const int potPin = SensorPin;
float pH_voltage, pH_corr;

// -- tds
float tdsValue = 0;

// -- ec
float ecVoltage, ecValue;
DFRobot_EC ec;

// -- turbidity
float ntu;

// -- do
const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

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

void setup(void)
{
  Serial.begin(115200);  //Initialize serial
  pinMode(potPin,INPUT);
  sensors.begin(); // start up temperature sensor
  ec.begin(); // start up tds sensor
  // -- tds
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization
}

void loop(void)
{
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;

  sensors.requestTemperatures(); // get temperature
  // Get a new temperature reading
  float tempC = sensors.getTempCByIndex(0);

  // -- do
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;

  // -- tds
  gravityTds.setTemperature(tempC);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value

  // -- turbidity
  int sensorValue = analogRead(TU_PIN);
  float tb_voltage = sensorValue * (5.0 / 1024.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  tb_voltage = round_to_dp(tb_voltage,2);
    if(tb_voltage < 2.5){
      ntu = 3000;
    }else{
      ntu = -1120.4*sqrt(tb_voltage)+ 5742.3*tb_voltage - 4352.9; 
    }
  
  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(potPin);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
      pHValue = 3.5*voltage;
      pH_voltage = voltage / 327.68;
      pH_corr = pHValue - Alpha * (T + tempC) * pH_voltage; // pH value with temperature compensation

      ecVoltage = analogRead(EC_PIN)/1024.0*5000;   // read the voltage
      ecValue =  ec.readEC(ecVoltage,tempC);  // convert voltage to EC with temperature compensation

      samplingTime=millis();
  }

  Serial.print("Temperature (ºC): ");
  Serial.print(tempC);
  //Serial.print(" | pH (measured): ");
  //Serial.print(pHValue);
  Serial.print(" | pH (corrected): ");
  Serial.print(pH_corr);  
  Serial.print(" | EC (ms/cm): ");
  Serial.print(ecValue,2);
  Serial.print(" | DO (mg/L): " + String((readDO(ADC_Voltage, tempC) / 1000.0)));
  Serial.print(" | TDS (ppm): ");
  Serial.print(tdsValue, 0);
  Serial.print(" | Turbidity (NTU): ");
  Serial.println(ntu, 0);
  
  // if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  // {
  //   Serial.print("Voltage:");
  //       Serial.print(voltage,2);
  //       Serial.print("    pH value: ");
  //   Serial.println(pHValue,2);
  //       digitalWrite(LED,digitalRead(LED)^1);
  //       printTime=millis();
  // }
}

// -- stuff for the pH calculation
double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}

// -- stuff for the turbidity calculation
float round_to_dp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}