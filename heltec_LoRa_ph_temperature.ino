#include "LoRaWan_APP.h"
#include <OneWire.h>
#include <DallasTemperature.h> // DS18B20 library

// -- Temperature
#define ONE_WIRE_BUS 7 // data wire is plugged to GPIO7
#define T 273.15               // degrees Kelvin
#define Alpha 0.05916          // alpha
// -- pH Value
#define Offset 0.10 
#define LED 13
#define samplingInterval 50
#define printInterval 800
#define ArrayLenth  40    //times of collection

/* OTAA para*/
uint8_t devEui[] = {  };
uint8_t appEui[] = {  }; // joinEUI
uint8_t appKey[] = {  };

/* ABP para*/
uint8_t nwkSKey[] = {  };
uint8_t appSKey[] = {  };
uint32_t devAddr =  ( uint32_t );

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 60000; // 60,000 = 1 minute

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
uint8_t confirmedNbTrials = 4;

// -- Temperature
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// -- pH Value
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;
const int phPin=6; // reading the analog value via pin GPIO6
float ph;
float Value=0;
// define params for temperature-based pH correction
float pHvoltage, pHcorr;

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */

  // TEMPERATURE
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it

  // PH VALUE
  pinMode(phPin,INPUT); // get the pH output
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;
  
  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(phPin);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*3.3/6306.0; // 6*1024 + 2*64 + 32
      pHValue = 3.3*voltage;
      pHvoltage = voltage / 327.68;
      pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage;
      samplingTime=millis();
  }

  // INTEGER CONVERSIONS
  int int_temp = tempC * 100; //remove comma
  int int_ph = pHcorr * 100;

  // DATA SIZES
  appDataSize = 4;
  appData[0] = int_temp >> 8;
  appData[1] = int_temp;
  appData[2] = int_ph >> 8;
  appData[3] = int_ph;
}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ 


void setup() {
  Serial.begin(115200);
  Mcu.begin();
  deviceState = DEVICE_STATE_INIT;
}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}

double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to averaging!/n");
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
