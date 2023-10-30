#include <WiFi.h>
#include "ThingSpeak.h"
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "FRITZ!Box 7490";   // your network SSID (name) 
const char* password = "91313742298868125947";   // your network password

WiFiClient  client;

unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "R2W7EJV0H1H17EFL";

#define Offset 0.20 
#define LED 13
#define samplingInterval 50
#define printInterval 800
#define ArrayLenth  40    //times of collection
#define ONE_WIRE_BUS 8 // pin the temperature sensor is connected to
#define T 273.15               // degrees Kelvin
#define Alpha 0.05916          // alpha

// Setup a oneWire instance to communicate with a OneWire device
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 15000;

// Variable to hold pH readings
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;
const int potPin=6;
float pH_voltage, pH_corr;

void setup() {
  Serial.begin(115200);  //Initialize serial
  pinMode(potPin,INPUT);
  sensors.begin();
  
  WiFi.mode(WIFI_STA);   
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {
  
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;
  
  if ((millis() - lastTime) > timerDelay) {

    sensors.requestTemperatures(); // get temperature
    // Get a new temperature reading
    float tempC = sensors.getTempCByIndex(0);
    
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }

    if(millis()-samplingTime > samplingInterval)
    {
        pHArray[pHArrayIndex++]=analogRead(potPin);
        if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
        voltage = avergearray(pHArray, ArrayLenth)*3.3/6656.0;
        pHValue = 3.3*voltage+Offset;
        pH_voltage = voltage / 327.68;
        pH_corr = pHValue - Alpha * (T + tempC) * pH_voltage;
        samplingTime=millis();
    }

    Serial.print("Temperature (ºC): ");
    Serial.print(tempC);
    Serial.print(" | pH (corrected): ");
    Serial.println(pH_corr);

    // set the fields with the values
    ThingSpeak.setField(1, tempC);
    ThingSpeak.setField(2, pH_corr);
    
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
  }
}

double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  
  if (number<=0) {
    Serial.println("Error number for the array to averaging!/n");
    return 0;
  }
  
  if (number<5) {   //less than 5, calculated directly statistics
    for (i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  } else {
    if (arr[0]<arr[1]) {
      min = arr[0];max=arr[1];
    }
    else {
      min=arr[1];max=arr[0];
    }
    for (i=2;i<number;i++) {
      if (arr[i]<min) {
        amount+=min;        //arr<min
        min=arr[i];
      } else {
        if (arr[i]>max) {
          amount+=max;    //arr>max
          max=arr[i];
        } else {
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}