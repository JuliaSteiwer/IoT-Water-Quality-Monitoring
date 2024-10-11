// Calibrate DFRobot pH sensor SKU:SEN0161 with Arduino UNO

#define phPin A0            //pH meter Analog output to Arduino Analog Input 0
#define Offset 0.00            //deviation compensate
#define LED 13
#define T 273.15               // degrees Kelvin
#define Alpha 0.05916          // alpha
#define tempC 20            // define static temperature (in Celsius) instead of using temperature sensor

float Value = 0;
float pHcorr, pHvoltage; // corrected pH value

void setup() {
  // put your setup code here, to run once:
  pinMode(LED,OUTPUT);
  Serial.begin(9600);
  Serial.println("pH meter experiment!");
}

void loop() {
  // put your main code here, to run repeatedly:
  pinMode(phPin,INPUT); // get the pH output
  static float pHValue,voltage;
  
  Value = analogRead(phPin);
  voltage = Value * (5.0 / 1024.0);
  pHValue = 3.5 * voltage; // + Offset if there is any.
  pHvoltage = voltage / 327.68;
  pHcorr = pHValue - Alpha * (T + tempC) * pHvoltage; // pH value, temperature-corrected

  Serial.print("Voltage pH: ");
  Serial.print(voltage);
  Serial.print(" | pH Value: ");
  Serial.print(pHValue);
  Serial.print(" | pH Value (corrected): ");
  Serial.println(pHcorr);

  delay(1000);
}
