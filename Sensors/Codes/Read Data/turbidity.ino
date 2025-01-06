// Julia Steiwer, 29. Dec. 2024
// Code for reading out data from SEN0189 turbidity sensor by DFRobot.
// Please read my notes carefully I know I wrote a lot but it's important!

// Turbidity
#define TU_PIN 2
float ntu;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Getting turbidity values: ");
}

void loop() {
  // put your main code here, to run repeatedly:

  // Turbidity
  int sensorValue = analogRead(TU_PIN);
  Serial.println(sensorValue); // just the analog reading
  float tb_voltage = sensorValue * (3.3 / 4096.0); // convert analog reading to voltage
  // the original code by DFRobot only throws out the voltage but we want the turbidity in NTU
  float multi = 100.0;
  tb_voltage = (tb_voltage * multi) / multi;
  if(tb_voltage < 1.83333333325){ // change from 2.5 to 1.83333333325 because instead of max 4.5V we only have max 3.3V now. Why? The sensor, when powered with 5V returns an output of maximum 4.5V, with the voltage being higher when the water is clearer. So we placed a 100 Ohm resistance to crack the 4.5V down to 3.3V, which is what the GPIO pin of the ESP32 can tolerate.
    ntu = 3000;
  }else{
    //ntu = -1120.4*(tb_voltage*tb_voltage)+ 5742.3*tb_voltage - 4352.9; // original curve equation as seen on DFRobot's Wiki. Note that you need to figure out these values empirically.
    ntu = -0.000487 * tb_voltage + 3.29; // I got this equation by taking the 2.5V and 4.5V values from DFRobot's image, convert the voltages to a max of 3.3V, and then put a line through that. You could use Formazin solutions for calibration but they're awfully pricey.
  }

  Serial.print("Turbidity [NTU]: ");
  Serial.println(ntu);

}

// Julia Steiwer, 29. Dec. 2024
