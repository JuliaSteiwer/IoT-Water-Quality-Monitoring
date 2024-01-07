#include "Arduino.h"
#include "LoRaWan_APP.h"
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
void setup()
{
	Serial.begin(115200);
	delay(100);
	lora_init();
	delay(1000); 
}


void loop()
{
 	VextOFF();
	Radio.Sleep();
	SPI.end();
	pinMode(RADIO_DIO_1,ANALOG);
	pinMode(RADIO_NSS,ANALOG);
	pinMode(RADIO_RESET,ANALOG);
	pinMode(RADIO_BUSY,ANALOG);
	pinMode(LORA_CLK,ANALOG);
	pinMode(LORA_MISO,ANALOG);
	pinMode(LORA_MOSI,ANALOG);
	esp_sleep_enable_timer_wakeup(6000*1000*(uint64_t)1000);
	esp_deep_sleep_start();
}
