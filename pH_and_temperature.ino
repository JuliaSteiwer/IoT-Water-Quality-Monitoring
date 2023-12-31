#include <OneWire.h>
#include <DallasTemperature.h>

#define Offset 0.10 
#define LED 13
#define samplingInterval 50
#define printInterval 800
#define ArrayLenth  40    //times of collection
#define ONE_WIRE_BUS 7
#define T 273.15               // degrees Kelvin
#define Alpha 0.05916          // alpha

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0;
const int potPin=6;
float ph;
float Value=0;
// define params for temperature-based pH correction
float pHvoltage, pHcorr;
 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(potPin,INPUT);
  // Start up the temperature library
  sensors.begin();
  delay(1000);
}
void loop(){

  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue,voltage;

  // call sensors.requestTemperatures() to issue a global temperature 
  sensors.requestTemperatures(); // Send the command to get temperatures
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);

  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(potPin);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*3.3/6306.0; // 6*1024 + 2*64 + 32
      pHValue = 3.3*voltage;
      pHvoltage = voltage / 327.68;
      pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage;
      samplingTime=millis();
  }
  if(millis() - printTime > printInterval)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    Serial.print("Voltage:");
        Serial.print(voltage,2);
        Serial.print("    Temperature: ");
    Serial.print(tempC,2);   
        Serial.print("    pH value: ");
    Serial.print(pHValue,2);
        Serial.print("    pH_tempCorr: ");
    Serial.println(pHcorr,2);    
        digitalWrite(LED,digitalRead(LED)^1);
        printTime=millis();
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
