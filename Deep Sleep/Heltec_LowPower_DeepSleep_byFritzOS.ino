// Code for sending the Heltec WiFi LoRa V3 board into deep sleep.
// Data is sent to TTN when the board wakes up.
// Code was kindly provided by GitHub user FritzOS.
// Original issue with code can be found here: 
// https://github.com/JuliaSteiwer/IoT-Water-Quality-Monitoring/issues/1#issue-2075032778

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
uint8_t devEui[] = { 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 }; // put your devEui here
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 }; // put your appKey here

/* ABP para, not used in this case*/
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint32_t devAddr =  ( uint32_t )0x00000000;

/*LoraWan channelsmask*/
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

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
