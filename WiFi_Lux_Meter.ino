#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MLX_ADDR 0x5A
Adafruit_MLX90614 mlx = Adafruit_MLX90614(MLX_ADDR);

const char* ssid =  "your wifi ssid here";
const char* password = "your wifi password here";

const char* host = "WiFi_Lux_Meter";

uint8_t gain = 1;

uint16_t lux[3];
float lumens;
float tempC = 0;

uint8_t sensor_error = 0;
uint8_t mlx_found = 0;

float divisor = 1.0;

ESP8266WebServer server(80);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

void set_divisor(float d) {
  EEPROM.put(0,d);
  EEPROM.commit();
  Serial.print("EEPROM now contains: ");
  float test;
  EEPROM.get(0,test);
  Serial.println(test);
}

float get_divisor() {
  float d;
  EEPROM.get(0,d);
  Serial.print("EEPROM contains: ");
  Serial.println(d);
  if(d == 0 || isnan(d)) {d = 1;} // don't let the divisor be 0 (or "nan" not a number)
  return d;
}

void getConnected() {
  Serial.print("Connecting to ");
  Serial.print(ssid);
  display.print("WiFi:"); display.display();
  WiFi.hostname(host);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print("."); display.display();
  }
  Serial.print("WiFi connected, IP address is ");
  Serial.println(WiFi.localIP());
  display.println(" connected!"); 
  display.print("IP Addr:");
  display.print(WiFi.localIP()); display.display();
}

void updateData() {
  // rotate the array and put on a new value
  lux[2] = lux[1];
  lux[1] = lux[0];
  lux[0] = tsl.getLuminosity(TSL2591_VISIBLE);
  Serial.print("getLuminosity: ");
  Serial.print(lux[0]);

  uint16_t tmp[3];
  uint16_t foo;

  // copy the current values
  tmp[0] = lux[0];
  tmp[1] = lux[1];
  tmp[2] = lux[2];

  // rudimentary bubble sort
  if (tmp[0] > tmp[1]) {
    // swap the values
    foo = tmp[1];
    tmp[1] = tmp[0];
    tmp[0] = foo;
  }
  if (tmp[1] > tmp[2]) {
    // swap the values
    foo = tmp[2];
    tmp[2] = tmp[1];
    tmp[1] = foo;
  }
  if (tmp[0] > tmp[1]) {
    // swap the values
    foo = tmp[1];
    tmp[1] = tmp[0];
    tmp[0] = foo;
  }

  // now that the array is sorted, the mean will be the central value
  if (tmp[1] == 65535) {
    lumens = 0;
  }
  else {
    lumens = tmp[1] / divisor / gain;
    if (lumens >= 100) {
      lumens = round(lumens); // round to 0 decimal points
    }
    else {
      lumens = ((float)((int)(lumens * 10 + 0.5))) / 10; // round to 1 decimal point
    }
  }

  Serial.print(", lumens: ");
  Serial.println(lumens);

  // if we're less than 10 lumens, use 25x gain (and ensure the next value returned is the median)
  if (gain == 1 && lumens < 10) {
    gain = 25;
    tsl.setGain(TSL2591_GAIN_MED);
    lux[0] = 0;
    lux[1] = 65535;
  }
  else if (gain > 1 && (lumens >= 10 || lux[0] == 0)) {  // >=10 lumens or we're saturated
    gain = 1;
    tsl.setGain(TSL2591_GAIN_LOW);
    lux[0] = 0;
    lux[1] = 65535;
  }

}

void handleRoot() {
  
  if(sensor_error) {
    server.send(200, "text/plain", "No sensor found ... check your wiring?");
  }
  else {
    if (server.hasArg("divisor")) { // set a new lux-lumens divisor
      divisor = server.arg("divisor").toFloat();
      set_divisor(divisor);
      server.send(200, "text/plain", "New divisor set: " + (String)divisor);
    }
    else {
      server.send(200, "text/plain", "Go to /read to get a reading\n\nGo to /lumens to get just the lumens reading\n\nUse ?divisor=#.## on this page to set a new factor");
    }
  }
}

void handleRead() { // send lumens and temperature
  
  if(sensor_error) {
    server.send(200, "text/plain", "No sensor found ... check your wiring?");
  }
  else {
      server.send(200, "text/plain", String(lumens, (lumens <= 10 ? 1 : 0)) + "," + (String)tempC);
  }
}

void handleLumens() {  // send just the lumens reading
  
  if(sensor_error) {
    server.send(200, "text/plain", "No sensor found ... check your wiring?");
  }
  else {
      server.send(200, "text/plain", (String)lumens);
  }
}

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(4);  // a float needs 4 bytes
  divisor = get_divisor();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setRotation(2);  // flip display upsidedown
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  getConnected();

  server.on("/", handleRoot);
  server.on("/read", handleRead);
  server.on("/lumens",handleLumens);
  server.begin();
  Serial.println("HTTP server started");
  display.println("HTTP server started"); display.display();

  if (tsl.begin()) {
    Serial.println("Found a TSL2591 sensor");
    display.println("TSL2591 sensor found"); display.display();
  }
  else {
    sensor_error = 1;
    Serial.println("TSL2591 not found ... check your wiring?");
    display.println("TSL2591 not found!"); display.display();
  }

  gain = 1;
  tsl.setGain(TSL2591_GAIN_LOW);

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  Wire.beginTransmission(MLX_ADDR);
  if (Wire.endTransmission() == 0) { // device found!
    mlx_found = 1;
    mlx.begin(); // initialize the MLX90614 sensor
    Serial.println("MLX90614 found!");
    display.println("MLX90614 sensor found"); display.display();
  }
  else {
    Serial.println("MLX90614 not found ... check your wiring?");
    display.println("MLX90614 not found!"); display.display();
  }

  delay(3000); // Pause for 3 seconds to display setup info
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { getConnected(); }
  server.handleClient();
  if(!sensor_error) { updateData(); } // this will block for the length of the integration time

  int16_t  x1, y1;
  uint16_t w, h;

  display.clearDisplay();
  display.setFont(&FreeSansBold18pt7b);
  String lumens_str = String(lumens, (lumens <= 10 ? 1 : 0));
  display.getTextBounds(lumens_str, 0, 28, &x1, &y1, &w, &h);
  display.setCursor(display.width() - w - 5, 28);
  display.println(lumens_str);
  if (mlx_found) {
    tempC = mlx.readObjectTempC();
    display.setCursor(0,63);
    display.print(tempC,0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(display.getCursorX(), 52);
    display.print(" *C");
  }
  display.setFont(&FreeSansBold9pt7b);
  display.getTextBounds((gain > 1 ? "x" : "") + (String)divisor, 0, 63, &x1, &y1, &w, &h);
  display.setCursor(display.width() - w - 5, 63);
  display.print((gain > 1 ? "x" : "") + (String)divisor);
  display.display();
}
