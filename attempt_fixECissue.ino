/*
* Example: Send battery voltage to TTN 
* tested with HELTEC WiFi LoRa 32(V3):
*     power consumption when sleeping: 0.019 mA
* tested with HELTEC Wireless Stick(V3):
*     power consumption when sleeping: 0.019 mA
*
* put your devEui and appKey in lines 80 and 82 !
*/
#include "Arduino.h"
#include <Wire.h>         
#include "HT_SSD1306Wire.h"
#include <LoRaWan_APP.h>
#include <CayenneLPP.h>
// sensor specific libraries
#include <OneWire.h>
#include <DallasTemperature.h> // DS18B20 library
//#include "DFRobot_EC.h"
#include <EEPROM.h>

// SENSOR DEFINITIONS
// -- Temperature
#define ONE_WIRE_BUS 7 // data wire is plugged to GPIO7
#define T 273.15               // degrees Kelvin
#define Alpha 0.05916          // alpha
// -- pH Value
#define Offset 0.10 
#define LED 13
// -- TDS Value
#define tdsPin 5
// -- EC
#define EC_PIN 4
#define RES2 820.0
#define ECREF 200.0
// -- DO
#define DO_PIN 3
#define VREF 5000    //VREF (mv)
#define ADC_RES 4096 //ADC Resolution
#define TWO_POINT_CALIBRATION 1
//Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (670) //mv
#define CAL1_T (8)   //℃
//Two-point calibration needs to be filled CAL2_V and CAL2_T
//CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1310) //mv
#define CAL2_T (24)   //℃
// -- Turbidity
#define TU_PIN 2

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
// -- end sensor definitions

//#define TRACE         // print debugging information
#ifdef TRACE
  uint32_t loop_counter;
#endif

#define MAXTIMETTNLOOP 180000     // after MAXTIMETTNLOOP milliseconds, it is assumed that the TTN connection was unsuccessful.
uint32_t startloopTTN;            // millis() at which loop_TTN() was started

//-------------------------------------------------------------------------------------------------------------------
//--  Display -- Display -- Display -- Display -- Display -- Display -- Display -- Display -- Display -- Display   --
//-------------------------------------------------------------------------------------------------------------------
extern SSD1306Wire display; // use display defined in LoRaWan_APP.h

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

//-------------------------------------------------------------------------------------------------------------------
//-- check battery level -- check battery level -- check battery level -- check battery level -- check battery level 
//-------------------------------------------------------------------------------------------------------------------
#define Read_VBAT_Voltage   1
#define ADC_CTRL           37
#define ADC_READ_STABILIZE 10       // in ms (delay from GPIO control and ADC connections times)

float readBatLevel() {
    #ifdef TRACE
      Serial.println("readBatLevel");
    #endif
    
    pinMode(ADC_CTRL,OUTPUT);
    digitalWrite(ADC_CTRL, LOW);                
    delay(ADC_READ_STABILIZE);                  // let GPIO stabilize
    int analogValue = analogRead(Read_VBAT_Voltage);

    #ifdef TRACE
      Serial.println("BatLevel = " + String(analogValue));
    #endif

    float voltage = 0.00403532794741887 * analogValue;
    #ifdef TRACE
      Serial.println("Voltage = " + String(voltage));
    #endif
    
    return voltage;
}

//-------------------------------------------------------------------------------------------------------------------
//-- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN -- TTN ----------
//-------------------------------------------------------------------------------------------------------------------

/* OTAA para*/
uint8_t devEui[] = {  }; // put your devEui here
uint8_t appEui[] = {  };
uint8_t appKey[] = {  }; // put your appKey here

/* ABP para, not used in this case*/
uint8_t nwkSKey[] = {  };
uint8_t appSKey[] = { };
uint32_t devAddr =  ( uint32_t );

/*LoraWan channelsmask*/
uint16_t userChannelsMask[6]={  };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 60000;  // 1 minute

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 8;

// DEFINE SENSOR VARIABLES
// -- Temperature
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// -- pH Value
// int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
// int pHArrayIndex=0;
const int phPin=6; // reading the analog value via pin GPIO6
float ph;
float Value=0;
// define params for temperature-based pH correction
float pHvoltage, pHcorr;

// -- TDS Value
int tdsSensorValue = 0;
float tdsValue = 0;
float tdsVoltage = 0;

// -- EC
float ecVoltage,ecValue;
//DFRobot_EC ec;
float kvalue = 0.996;

// -- turbidity
float ntu;
// -- end sensor variable definitions

//-------------------------------------------------------------------------------------------------------------------
//-- prepareTxFrame -- prepareTxFrame -- prepareTxFrame -- prepareTxFrame -- prepareTxFrame -- prepareTxFrame -------
//-------------------------------------------------------------------------------------------------------------------
static void prepareTxFrame( uint8_t port)
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
  CayenneLPP lpp(51);
  lpp.reset();

  int last_rssi = 0;
  int last_snr = 0;

  // void addGPS(uint8_t channel, float latitude, float longitude, int32_t altitude);
  // channel: The channel number to associate with this GPS data.
  // latitude: The latitude value in decimal degrees.
  // longitude: The longitude value in decimal degrees.
  // altitude: The altitude value in centimeters. This parameter represents the height above the sea level.
  lpp.addAnalogInput(1, readBatLevel()); // Channel 1: Voltage of LiPo

  appDataSize = lpp.getSize();
  memcpy(appData, lpp.getBuffer(), appDataSize);

  // Sensor stuff goes here
  // TEMPERATURE
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // PH VALUE
  pinMode(phPin,INPUT); // get the pH output
  static float pHValue,voltage;

  Value = analogRead(phPin);
  voltage = Value * (5.0/6144.0); // 6306.0; 4095.0
  //pHValue = 3.3*voltage+Offset;
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
  //ec.begin();
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
  //ecValue =  ec.readEC(ecVoltage,tempC);
  // Serial.println(ecValue,2);

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

  // INTEGER CONVERSIONS
  int int_temp = tempC * 100; //remove comma
  int int_ph = pHcorr * 100;
  int int_tds = tdsValue * 100;
  int int_ec = ecValue * 100;
  int int_do = DO * 100;
  int int_ntu = ntu * 100;

  // DATA SIZES
  appDataSize = 12;
  appData[0] = int_temp >> 8;
  appData[1] = int_temp;
  appData[2] = int_ph >> 8;
  appData[3] = int_ph;
  appData[4] = int_tds >> 8;
  appData[5] = int_tds;
  appData[6] = int_ec >> 8;
  appData[7] = int_ec;
  appData[8] = int_do >> 8;
  appData[9] = int_do;
  appData[10] = int_ntu >> 8;
  appData[11] = int_ntu;
  // end sensor stuff

  #ifdef TRACE
    Serial.println("appDataSize = " + String(appDataSize));
    Serial.println();
  #endif  
}

//-------------------------------------------------------------------------------------------------------------------
//-- loop_TTN --  loop_TTN -- loop_TTN -- loop_TTN -- loop_TTN -- loop_TTN -- loop_TTN -- loop_TTN -- loop_TTN ------
//-------------------------------------------------------------------------------------------------------------------
void loop_TTN()
{
  #ifdef TRACE
    Serial.println("ttn_loop " + String(loop_counter++));    
  #endif
  
  switch( deviceState )
    {
      case DEVICE_STATE_INIT:
      {
        #ifdef TRACE
          Serial.println("DEVICE_STATE_INIT");    
        #endif

        extern bool IsLoRaMacNetworkJoined; // enforce new join to TTN
        IsLoRaMacNetworkJoined = false;

        LoRaWAN.init(loraWanClass,loraWanRegion);
        break;
      }
      case DEVICE_STATE_JOIN:
      {
        #ifdef TRACE
          Serial.println("DEVICE_STATE_JOIN");    
        #endif

        display.clear();
        display.drawString(0,10,"join-TTN");
        display.display();
      
        LoRaWAN.join();

        #ifdef TRACE
          Serial.println("DEVICE_STATE_JOIN end");    
        #endif
        
        break;
      }
      case DEVICE_STATE_SEND:
      {
        #ifdef TRACE
          Serial.println("DEVICE_STATE_SEND");    
        #endif

        display.clear();
        display.drawString(0,10, "send-TTN");
        display.display();

        prepareTxFrame(appPort);
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
      case DEVICE_STATE_CYCLE:
      {
        // Schedule next packet transmission    
        #ifdef TRACE
          Serial.println("DEVICE_STATE_CYCLE");    
        #endif

        txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND );
        LoRaWAN.cycle(txDutyCycleTime);
        
        #ifdef TRACE
          Serial.println("txDutyCycleTime = " + String(txDutyCycleTime));    
        #endif
        
        deviceState = DEVICE_STATE_SLEEP;
        break;          
      }
      case DEVICE_STATE_SLEEP:
      { 
        extern uint8_t ifDisplayAck;  // ifDisplayAck is defined in LoRaWan_APP.h stack
                                      // it indicates the transmission has been completed
        
        LoRaWAN.sleep(loraWanClass);

        #ifdef TRACE
          Serial.println("DEVICE_STATE_SLEEP, ifDisplayAck = " + String(ifDisplayAck));    
        #endif

        if (ifDisplayAck == 1) // message has been sent out
        {
          LoRaWAN.displayAck();
          extern int revrssi, revsnr; //indication of signal strength, defined in LoRaWan_APP.h stack 

          #ifdef TRACE
            Serial.println("rssi = " + String(revrssi) + ", snr = " + String(revsnr));    
          #endif
        }

        break;
      }
      default:
      {
        #ifdef TRACE
          Serial.println("default");    
        #endif

        deviceState = DEVICE_STATE_INIT;
        break;
      }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//-- Setup -- Setup -- Setup -- Setup -- Setup -- Setup -- Setup -- Setup -- Setup -- Setup -- Setup -- Setup -------
//-------------------------------------------------------------------------------------------------------------------
void setup() {
  // initialize OLED
  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_16);

  //start serial interface
  Serial.begin(115200);
  Serial.println("Start");

  // initialize OLED display
  display.init();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  // prepare LoRaWAN
  Mcu.begin();
  deviceState = DEVICE_STATE_INIT;

  startloopTTN = millis();

  #ifdef TRACE
    loop_counter = 1;
  #endif

  Serial.println("Setup fertig");
}

//-------------------------------------------------------------------------------------------------------------------
//-- loop -- loop -- loop -- loop -- loop -- loop -- loop -- loop -- loop -- loop -- loop -- loop -- loop -- loop ---
//-------------------------------------------------------------------------------------------------------------------
void loop() {
  // very rarely does the TTN connection go into a stale status and requires a restart
  // we assume this after MAXTIMETTNLOOP milliseconds
  if((millis() - startloopTTN) > MAXTIMETTNLOOP) 
  {
    Serial.println();
    Serial.println("XXX     it is assumed that the TTN connection was unsuccessful        XXX");
    Serial.println();

    ESP.restart();
  }

  loop_TTN();
}
