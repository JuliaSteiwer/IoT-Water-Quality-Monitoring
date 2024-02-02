// VEXT goes to 3.3V when awake and 0V when asleep
// Except for TDS and turbidity, all sensor values are correct.
// From VEXT a step-up boosts the voltage to 5V to power the sensors.
// Wake-up is somehow never caused by the timer (60s) but does wake up every 60s.

// temperature libraries
#include <OneWire.h>
#include <DallasTemperature.h>
// ec library
#include <EEPROM.h>

// TEMPERATURE SENSOR
#define ONE_WIRE_BUS 7 // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// PH SENSOR
#define T 273.15               // degrees Kelvin → for temperature correction
#define Alpha 0.05916          // alpha → for temperature correction
#define Offset 0.10 
#define LED 13
const int phPin=6; // reading the analog value via pin GPIO6
float ph;
float Value=0;
float pHvoltage, pHcorr; // define params for temperature-based pH correction

// TDS SENSOR
#define tdsPin 5
int tdsSensorValue = 0;
float tdsValue = 0;
float tdsVoltage = 0;

// EC SENSOR
#define EC_PIN 4
#define RES2 820.0
#define ECREF 200.0
float ecVoltage,ecValue;
float kvalue = 0.996;

// DO SENSOR
#define DO_PIN 3
#define VREF 5000    //VREF (mv)
#define ADC_RES 4096 //ADC Resolution
#define TWO_POINT_CALIBRATION 1
//Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (670) // mV
#define CAL1_T (8)   // °C
//Two-point calibration needs to be filled CAL2_V and CAL2_T
//CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1310) // mV
#define CAL2_T (24)   // °C

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

uint8_t Temperaturet;
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

// TURBIDITY SENSOR
#define TU_PIN 2
float ntu;

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup(){
  VextON();
  Serial.begin(115200);

  delay(1000); //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  // TEMPERATURE
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // PH VALUE
  pinMode(phPin,INPUT); // get the pH output
  static float pHValue,voltage;
  Value = analogRead(phPin);
  voltage = Value * (5.0/6144.0); // 6306.0; 4095.0
  pHValue = 3.5*voltage;
  pHvoltage = voltage / 327.68;
  pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage; 

  // TDS VALUE
  tdsSensorValue = analogRead(tdsPin);
  tdsVoltage = tdsSensorValue*(5.0/4096.0); //Convert analog reading to Voltage
  float compensationCoefficient=1.0+0.02*(tempC-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compVoltage=tdsVoltage/compensationCoefficient;
  tdsValue=(133.42/compVoltage*compVoltage*compVoltage - 255.86*compVoltage*compVoltage + 857.39*compVoltage)*0.5;

  // EC VALUE
  ecVoltage = analogRead(EC_PIN)/1024.0*5000;
  float rawEC = 1000*ecVoltage/RES2/ECREF;
  float valueTemp = rawEC * kvalue; // 1.0 is the k-value (K = 1)
  if(valueTemp > 2.5){
    kvalue = 0.996;
  }else if(valueTemp < 2.0){
    kvalue = 0.996;
  }
  float value = rawEC * kvalue;             //calculate the EC value after automatic shift
  value = value / (1.0+0.0185*(tempC-25.0));  //temperature compensation
  float ecValue = value;

  // DO VALUE
  Temperaturet = tempC;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  int do_raw = readDO(ADC_Voltage, Temperaturet);
  float DO = do_raw / 1000.0; // convert DO unit to mg/L

  // TURBIDITY
  int sensorValue = analogRead(TU_PIN);
  float tb_voltage = sensorValue * (5.0 / 1024.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float multiplier = powf( 10.0f, 2 );
  tb_voltage = roundf( tb_voltage * multiplier ) / multiplier;
  if(tb_voltage < 2.5){
    ntu = 3000;
  }else{
    ntu = -1120.4*(tb_voltage*tb_voltage)+ 5742.3*tb_voltage - 4352.9; 
  }

  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.print(" | pH: ");
  Serial.print(pHcorr);
  Serial.print(" | TDS: ");
  Serial.print(tdsValue);
  Serial.print(" | EC: ");
  Serial.print(ecValue);
  Serial.print(" | DO: ");
  Serial.print(DO);
  Serial.print(" | Turbidity: ");
  Serial.println(ntu);

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  /*
  Next we decide what all peripherals to shut down/keep on
  By default, ESP32 will automatically power down the peripherals
  not needed by the wakeup source, but if you want to be a poweruser
  this is for you. Read in detail at the API docs
  http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
  Left the line commented as an example of how to configure peripherals.
  The line below turns off all RTC peripherals in deep sleep.
  */
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");

  /*
  Now that we have setup a wake cause and if needed setup the
  peripherals state in deep sleep, we can now start going to
  deep sleep.
  In the case that no wake up sources were provided but deep
  sleep was started, it will sleep forever unless hardware
  reset occurs.
  */
  Serial.println("Going to sleep now");
  //delay(1000);
  delay(2000);
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop(){
  //This is not going to be called
}
