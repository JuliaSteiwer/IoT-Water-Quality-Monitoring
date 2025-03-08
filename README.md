# IoT-Water-Quality-Monitoring
Monitoring water quality parameters using an IoT-based system.

<h2>Publications</h2>

> [!NOTE]
> My papers, posters etc. corresponding to this project will be uploaded in the Publications folder. As soon as one of them is published "officially", e.g. in a journal or open access site, I will also put the proper citation with the DOI here.

<h2>Resources</h2>
<h3>Tutorials</h3>
<p>I used <a href="https://www.aeq-web.com/heltec-lora32-v3-board-arduino-ide-lorawan-setup/">this tutorial</a> by Alex from AEQ-WEB for setting up LoRa with TTN and the Heltec Lora32 V3 board.</p>
<h3>Codes</h3>
<p>The code uses RadioLib by <a href="https://github.com/jgromes">Jan Gromeš (@jgromes)</a>, specifically the "<a href="https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Starter">LoRaWAN_Starter</a>" example. The contents of the .ino file were replaced with the content from the "<a href="https://github.com/radiolib-org/radiolib-persistence/blob/main/examples/LoRaWAN_ESP32/LoRaWAN_ESP32.ino">LoRaWAN_ESP32</a>" example of RadioLib Persistence by <a href="https://github.com/radiolib-org">@radiolib-org</a>, to ensure deep sleep during inactivity and enabling session persistence. The contents of the config.h file wered edited to support the SX1262 chip and match the LoRaWAN 1.0.4 configuration. The sensor codes for the pH, dissolved oxygen, turbidity, electrical conductivity, and total dissolved solids sensors are from <a href="https://github.com/DFRobot">@DFRobot</a>. The temperature sensor code is from <a href="https://starthardware.org/arduino-ds18b20-temperaturmessung-mit-digitalem-sensor/">this tutorial by Start Hardware</a>.</p>
<h3>Hardware</h3>
<p>The following hardware resources have been used for this project:</p>
<ul>
  <li>Microcontroller: Heltec WiFi LoRa 32 (v3) (<a href="https://heltec.org/project/wifi-lora-32-v3/">here</a>)</li>
  <li>Sensor suite: </li>
    <ul>
      <li>Dissolved Oxygen (DO) (<a href="https://www.dfrobot.com/product-1628.html">here</a>)</li>
      <li>Electrical Conductivity (EC) (<a href="https://www.dfrobot.com/product-1123.html">here</a>)</li>
      <li>pH (<a href="https://www.dfrobot.com/product-1025.html">here</a>)</li>
      <li>Temperature (<a href="https://www.dfrobot.com/product-1354.html">here</a>)</li>
      <li>Total Dissolved Solids (TDS) (<a href="https://www.dfrobot.com/product-1662.html">here</a>)</li>
      <li>Turbidity (<a href="https://www.dfrobot.com/product-1394.html">here</a>)</li>
    </ul>
  <li>SparkFun Sunny Buddy - MPPT Solar Charger (<a href="https://www.sparkfun.com/products/12885">here</a>)</li>
</ul>

> [!WARNING]
> The working code and the schematic are written with the use of the above hardware in mind. If you use other hardware, you will have to change the code and the wiring, otherwise, errors and false readings will occur!

<p>Further sensors that could enhance the system are:</p>
<ul>
  <li>BME280: Sensor for humidity, temperature, and air pressure. Useful inside of the sensor box to monitor if the electronics remain dry and in working temperature range.</li>
  <li>Flow meter to assess the river flow rate, useful to contextualize the other water parameters, especially for modeling purposes.</li>
  <li>Rainfall sensor to assess influences on flow rate.</li>
  <li>Water level sensor, both to contextualize the other parameters and for flood or drought warning purposes.</li>
</ul>

<h2>Acknowledgements</h2>
<p>Thanks to Steven Boonstoppel (@bns), Nick McCloud (@nmcc), and users @dstacer and @ksjh from the Heltec community for their help, feedback, and general input (threads <a href="http://community.heltec.cn/t/connecting-multiple-sensors-to-heltec-lora-v3-board-and-sending-the-data-to-thingspeak/14329">here</a> and <a href="http://community.heltec.cn/t/wifi-lora-32-v3-send-sensor-data-via-lora-then-go-back-to-deep-sleep/14392">here</a>).</p>
