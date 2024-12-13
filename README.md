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

<h2>Acknowledgements</h2>
<p>Thanks to Steven Boonstoppel (@bns), Nick McCloud (@nmcc), and users @dstacer and @ksjh from the Heltec community for their help, feedback, and general input (threads <a href="http://community.heltec.cn/t/connecting-multiple-sensors-to-heltec-lora-v3-board-and-sending-the-data-to-thingspeak/14329">here</a> and <a href="http://community.heltec.cn/t/wifi-lora-32-v3-send-sensor-data-via-lora-then-go-back-to-deep-sleep/14392">here</a>).</p>
