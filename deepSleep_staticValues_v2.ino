#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "esp_sleep.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */

/* OTAA para*/
uint8_t devEui[] = {  };
uint8_t appEui[] = {  }; // joinEUI
uint8_t appKey[] = {  };

/* ABP para*/
uint8_t nwkSKey[] = {  };
uint8_t appSKey[] = {  };
uint32_t devAddr =  ( uint32_t );

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={  };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

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

// boolean, which is true when a message is sent
bool messageSent = false;

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
  float temperature = 22.55;  //example 22.55 *C
  float humidity = 72.5;    //example 72.5 %

  int int_temp = temperature * 100; //remove comma
  int int_hum = humidity * 10; //remove comma

  appDataSize = 4;
  appData[0] = int_temp >> 8;
  appData[1] = int_temp;
  appData[2] = int_hum >> 8;
  appData[3] = int_hum;

  Serial.print("temperature: ");
  Serial.print(temperature);
  Serial.print(" | humidity: ");
  Serial.println(humidity);
}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ 

void lora_init(void)
{
  Mcu.begin();
  static RadioEvents_t RadioEvents;

  RadioEvents.TxDone = NULL;
  RadioEvents.TxTimeout = NULL;
  RadioEvents.RxDone = NULL;

  Radio.Init( &RadioEvents );

}

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

void setup() {
  Serial.begin(115200);
  Mcu.begin();
  lora_init();
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
      Serial.println("state = join");
      LoRaWAN.join();
      break;
    }

    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      messageSent = true;
      Serial.println("sent data");
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }

    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      Serial.println("enter sleep state. zzz");

      deviceState = DEVICE_STATE_SLEEP;

      break;
    }

    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      #if(messageSent == true)
        // Ignore this specific stuff for now because deep sleep of esp is the focus.
        // VextOFF();
        // Radio.Sleep();
        // SPI.end();
        // pinMode(RADIO_DIO_1,ANALOG);
        // pinMode(RADIO_NSS,ANALOG);
        // pinMode(RADIO_RESET,ANALOG);
        // pinMode(RADIO_BUSY,ANALOG);
        // pinMode(LORA_CLK,ANALOG);
        // pinMode(LORA_MISO,ANALOG);
        // pinMode(LORA_MOSI,ANALOG);
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
      #endif
      break;
    }

    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }

}

