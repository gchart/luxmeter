# luxmeter
ESP8266-based WiFi Lux Meter

These aren't full instructions, but should provide a good overview

Equipment needed:
* ESP8266 board (I used a Wemos D1 Mini)
* 0.96" OLED screen, i2c based with SSD1306 controller [such as this](https://www.amazon.com/dp/B06XRBTBTB/)
* TSL2591 Lux sensor [such as this](https://learn.adafruit.com/adafruit-tsl2591)
* MLX90614 IR Temperature sensor [such as this](https://www.amazon.com/gp/product/B07YZVDWWB/)
* Wires and/or perf board

Steps:
* Wire all components in parallel to the i2c pins of the ESP8266 board
* Use Ardunio IDE to load the software on the ESP8266 (don't forget to enter your wifi info)
* Connect the ESP8266 to power.  If all goes right, it should connect to wifi and the sensors and display that information on the screen
* You can now use a web browser to access the lux and temperature values (simply go to the IP address of your ESP8266)
* To log data, use a batch script, such as those attached, to store the readouts into a CSV file
* Don't forget to calibrate the lux meter (use a get parameter with the calibration factor when accessing the ESP8266's URL)
