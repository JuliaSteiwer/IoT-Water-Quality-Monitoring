
/*
ORIGINAL COMMENT FROM RADIOLIB PERSISTENCE EXAMPLE

This demonstrates how to save the join information in to permanent memory
so that if the power fails, batteries run out or are changed, the rejoin
is more efficient & happens sooner due to the way that LoRaWAN secures
the join process - see the wiki for more details.

This is typically useful for devices that need more power than a battery
driven sensor - something like a air quality monitor or GPS based device that
is likely to use up it's power source resulting in loss of the session.

The relevant code is flagged with a ##### comment

Saving the entire session is possible but not demonstrated here - it has
implications for flash wearing and complications with which parts of the 
session may have changed after an uplink. So it is assumed that the device
is going in to deep-sleep, as below, between normal uplinks.

Once you understand what happens, feel free to delete the comments and 
Serial.prints - we promise the final result isn't that many lines.

*/

#if !defined(ESP32)
  #pragma error ("This is not the example your device is looking for - ESP32 only")
#endif

// ##### load the ESP32 preferences facilites
#include <Preferences.h>
Preferences store;

// LoRaWAN config, credentials & pinmap
#include "config.h" 

#include <RadioLib.h>

// SENSOR LIBRARIES
// -- temperature
#include <OneWire.h>
#include <DallasTemperature.h>
// -- electrical conductivity (EC)
#include <EEPROM.h>

// Enable Pin
#define VReg 47

// SENSOR DEFINITIONS
// -- Temperature
#define ONE_WIRE_BUS 7         // pin where the data wire is plugged into
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// -- pH Value
#define T 273.15               // degrees Kelvin → for temperature correction
#define Alpha 0.05916          // alpha → for temperature correction
#define Offset 1.37            // calibrate sensor first, that sets the Offset value
#define LED 13
const int phPin = 6;           // reading the analog value via pin GPIO6
float ph;
float Value = 0;
float pHvoltage, pHcorr;        // define parameters for temperature-based pH correction

// -- total dissolved solids (TDS)
#define tdsPin 5
int tdsSensorValue = 0;
float tdsValue = 0;
float tdsVoltage = 0;

// -- electrical conductivity (EC)
#define EC_PIN 4
#define RES2 820.0
#define ECREF 200.0
float ecVoltage,ecValue;
float kvalue = 0.65;             // was 0.996 (sensor calibration gives you this value)

// -- dissolved oxygen (DO)
#define DO_PIN 3
#define VREF 3300                //VREF (mv)
#define ADC_RES 4096             //ADC Resolution
#define TWO_POINT_CALIBRATION 1
//Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (1400)            // mV
#define CAL1_T (27.81)           // °C
//Two-point calibration needs to be filled CAL2_V and CAL2_T (make sure to calibrate sensor first!)
//CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1160)            // mV
#define CAL2_T (15.69)           // °C

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

// -- turbidity
#define TU_PIN 2
float ntu;

// utilities & vars to support ESP32 deep-sleep. The RTC_DATA_ATTR attribute
// puts these in to the RTC memory which is preserved during deep-sleep
RTC_DATA_ATTR uint16_t bootCount = 0;
RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin = 0;
RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

// abbreviated version from the Arduino-ESP32 package, see
// https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/deepsleep.html
// for the complete set of options
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println(F("Wake from sleep"));
  } else {
    Serial.print(F("Wake not caused by deep sleep: "));
    Serial.println(wakeup_reason);
  }

  Serial.print(F("Boot count: "));
  Serial.println(++bootCount);      // increment before printing
}

// put device in to lowest power deep-sleep mode
void gotoSleep(uint32_t seconds) {
  esp_sleep_enable_timer_wakeup(seconds * 1000UL * 1000UL); // function uses uS
  Serial.println(F("Sleeping\n"));
  Serial.flush();

  esp_deep_sleep_start();

  // if this appears in the serial debug, we didn't go to sleep!
  // so take defensive action so we don't continually uplink
  Serial.println(F("\n\n### Sleep failed, delay of 5 minutes & then restart ###\n"));
  delay(5UL * 60UL * 1000UL);
  ESP.restart();
}

int16_t lwActivate() {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // setup the OTAA session information
  // node.beginOTAA(joinEUI, devEUI, nwkKey, appKey); // original
  node.beginOTAA(joinEUI, devEUI, NULL, appKey); // set nwkKey to NULL because we're using LoRaWAN 1.0.4

  Serial.println(F("Recalling LoRaWAN nonces & session"));
  // ##### setup the flash storage
  store.begin("radiolib");
  // ##### if we have previously saved nonces, restore them and try to restore session as well
  if (store.isKey("nonces")) {
    uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];										// create somewhere to store nonces
    store.getBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);	// get them from the store
    state = node.setBufferNonces(buffer); 															// send them to LoRaWAN
    debug(state != RADIOLIB_ERR_NONE, F("Restoring nonces buffer failed"), state, false);

    // recall session from RTC deep-sleep preserved variable
    state = node.setBufferSession(LWsession); // send them to LoRaWAN stack

    // if we have booted more than once we should have a session to restore, so report any failure
    // otherwise no point saying there's been a failure when it was bound to fail with an empty LWsession var.
    debug((state != RADIOLIB_ERR_NONE) && (bootCount > 1), F("Restoring session buffer failed"), state, false);

    // if Nonces and Session restored successfully, activation is just a formality
    // moreover, Nonces didn't change so no need to re-save them
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("Succesfully restored session - now activating"));
      state = node.activateOTAA();
      debug((state != RADIOLIB_LORAWAN_SESSION_RESTORED), F("Failed to activate restored session"), state, true);

      // ##### close the store before returning
      store.end();
      return(state);
    }

  } else {  // store has no key "nonces"
    Serial.println(F("No Nonces saved - starting fresh."));
  }

  // if we got here, there was no session to restore, so start trying to join
  state = RADIOLIB_ERR_NETWORK_NOT_JOINED;
  while (state != RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.println(F("Join ('login') to the LoRaWAN Network"));
    state = node.activateOTAA();

    // ##### save the join counters (nonces) to permanent store
    Serial.println(F("Saving nonces to flash"));
    uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];           // create somewhere to store nonces
    uint8_t *persist = node.getBufferNonces();                  // get pointer to nonces
    memcpy(buffer, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);  // copy in to buffer
    store.putBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE); // send them to the store
    
    // we'll save the session after an uplink

    if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
      Serial.print(F("Join failed: "));
      Serial.println(state);

      // how long to wait before join attempts. This is an interim solution pending 
      // implementation of TS001 LoRaWAN Specification section #7 - this doc applies to v1.0.4 & v1.1
      // it sleeps for longer & longer durations to give time for any gateway issues to resolve
      // or whatever is interfering with the device <-> gateway airwaves.
      uint32_t sleepForSeconds = min((bootCountSinceUnsuccessfulJoin++ + 1UL) * 60UL, 3UL * 60UL);
      Serial.print(F("Boots since unsuccessful join: "));
      Serial.println(bootCountSinceUnsuccessfulJoin);
      Serial.print(F("Retrying join in "));
      Serial.print(sleepForSeconds);
      Serial.println(F(" seconds"));

      gotoSleep(sleepForSeconds);

    } // if activateOTAA state
  } // while join

  Serial.println(F("Joined"));

  // reset the failed join count
  bootCountSinceUnsuccessfulJoin = 0;

  delay(1000);  // hold off off hitting the airwaves again too soon - an issue in the US
  
  // ##### close the store
  store.end();  
  return(state);
}

// setup & execute all device functions ...
void setup() {
  Serial.begin(115200);

  // enable pin stuff
  pinMode(VReg, OUTPUT);
  digitalWrite(VReg, HIGH);   // turn the LED on (HIGH is the voltage level)

  while (!Serial);  							// wait for serial to be initalised
  delay(2000);  									// give time to switch to the serial monitor
  Serial.println(F("\nSetup"));
  print_wakeup_reason();

  int16_t state = 0;  						// return value for calls to RadioLib

  // setup the radio based on the pinmap (connections) in config.h
  Serial.println(F("Initalise the radio"));
  state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initalise radio failed"), state, true);

  // activate node by restoring session or otherwise joining the network
  state = lwActivate();
  // state is one of RADIOLIB_LORAWAN_NEW_SESSION or RADIOLIB_LORAWAN_SESSION_RESTORED

  // ----- and now for the main event -----
  Serial.println(F("Sending uplink"));

  // this is the place to gather the sensor inputs
  // instead of reading any real sensor, we just generate some random numbers as example
  // uint8_t value1 = radio.random(100);
  // uint16_t value2 = radio.random(2000);

  // SENSOR INPUTS
  // -- DS18B20 Temperature
  sensors.begin(); // start up the sensor library
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0); // there's only one temperature sensor so we can index to it
  
  // -- pH
  pinMode(phPin,INPUT); // get the pH output
  static float pHValue,voltage;
  Value = analogRead(phPin);
  voltage = Value * (3.3/4096.0); // ESP32 analogRead output is 0 to 4095, so divide by 4096; 3.3 because the GPIO only received a max of 3.3
  pHValue = 3.5*voltage;
  pHvoltage = voltage / 327.68;
  pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage;
  
  // -- total dissolved solids
  // all put here because #include "GravityTDS.h" only works with Arduino UNO and not with ESP32
  tdsSensorValue = analogRead(tdsPin);
  tdsVoltage = tdsSensorValue*(3.3/4096.0);
  float compensationCoefficient=1.0+0.02*(tempC-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compVoltage=tdsVoltage/compensationCoefficient;
  tdsValue=(133.42/compVoltage*compVoltage*compVoltage - 255.86*compVoltage*compVoltage + 857.39*compVoltage)*0.5;
  
  // -- electrical conductivity (EC)
  ecVoltage = analogRead(EC_PIN)/4096.0*3300; // 1024.0*5000 für Arduino, 4096*3300 für ESP32
  float rawEC = 1000*ecVoltage/RES2/ECREF;
  float valueTemp = rawEC * kvalue; // 1.0 is the k-value (K = 1)
  if(valueTemp > 2.5){
    kvalue = 0.65; // was 0.996 (value dependent on sensor calibration)
  }else if(valueTemp < 2.0){
    kvalue = 0.65; // was 0.996 (value dependent on sensor calibration)
  }
  float value = rawEC * kvalue;             //calculate the EC value after automatic shift
  value = value / (1.0+0.0185*(tempC-25.0));  //temperature compensation
  float ecValue = value;

  // -- dissolved oxygen (DO)
  Temperaturet = tempC;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  int do_raw = readDO(ADC_Voltage, Temperaturet);
  float DO = do_raw / 1000.0; // convert DO unit to mg/L

  // -- turbidity
  int sensorValue = analogRead(TU_PIN);
  Serial.println(sensorValue);
  float tb_voltage = sensorValue * (3.3 / 4096.0); // convert analog reading
  float multi = 100.0;
  tb_voltage = (tb_voltage * multi) / multi;
  if(tb_voltage < 1.83333333325){ // change from 2.5 to 1.83333333325 because instead of max 4.5V we only have max 3.3V (to match ESP32 pins); use 100 Ohm resistance to get the max. output from 4.5V to 3.3V
    ntu = 3000;
  }else{
    //ntu = -1120.4*(tb_voltage*tb_voltage)+ 5742.3*tb_voltage - 4352.9; 
    // alte Gleichung: 4298 + -139x + -260x^2
    // ntu = 4298.0 - 139.0 * tb_voltage - 260.0 * (tb_voltage * tb_voltage);
    // neue Gleichung (01.12.2024): -4.87 * 10^-4 * x + 3.29
    ntu = -0.000487 * tb_voltage + 3.29;
  }

  // INTEGER CONVERSIONS
  int int_temp = tempC * 100; //remove comma
  int int_ph = pHcorr * 100;
  int int_tds = tdsValue * 100;
  int int_ec = ecValue * 100;
  int int_do = DO * 100;
  int int_ntu = ntu * 100;

  // build payload byte array
  // uint8_t uplinkPayload[3];
  // uplinkPayload[0] = value1;
  // uplinkPayload[1] = highByte(value2);   // See notes for high/lowByte functions
  // uplinkPayload[2] = lowByte(value2);
  uint8_t uplinkPayload[12];
  uplinkPayload[0] = int_temp >> 8;
  uplinkPayload[1] = int_temp;
  uplinkPayload[2] = int_ph >> 8;
  uplinkPayload[3] = int_ph;
  uplinkPayload[4] = int_tds >> 8;
  uplinkPayload[5] = int_tds;
  uplinkPayload[6] = int_ec >> 8;
  uplinkPayload[7] = int_ec;
  uplinkPayload[8] = int_do >> 8;
  uplinkPayload[9] = int_do;
  uplinkPayload[10] = int_ntu >> 8;
  uplinkPayload[11] = int_ntu;

  
  // perform an uplink
  state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));    
  debug((state < RADIOLIB_ERR_NONE) && (state != RADIOLIB_ERR_NONE), F("Error in sendReceive"), state, false);

  Serial.print(F("FCntUp: "));
  Serial.println(node.getFCntUp());

  // now save session to RTC memory
  uint8_t *persist = node.getBufferSession();
  memcpy(LWsession, persist, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
  
  // wait until next uplink - observing legal & TTN FUP constraints
  gotoSleep(uplinkIntervalSeconds);

}


// The ESP32 wakes from deep-sleep and starts from the very beginning.
// It then goes back to sleep, so loop() is never called and which is
// why it is empty.

void loop() {}
