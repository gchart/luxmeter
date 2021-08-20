/* 
 *  Much of the WebSockets code was borrowed from 
 *  https://randomnerdtutorials.com/esp8266-nodemcu-websocket-server-arduino/
 *  thanks to them for the tutorial!
 *
 *  Make sure to use the ESP8266 core available here: https://github.com/esp8266/Arduino
*/

#include <WiFiManager.h>  // Search Library Manager for "wifimanager" and install "WiFiManager by tzapu"
#include <EEPROM.h>
#include <ESPAsyncTCP.h>  // Download & import https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip
#define WEBSERVER_H
#include <ESPAsyncWebServer.h> // Download & import https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip

#include <Adafruit_Sensor.h>     // Arduino library: Adafruit Unified Sensor
#include <Adafruit_TSL2591.h>    // Arduino library: Adafruit TSL2591
#include <Adafruit_MLX90614.h>   // Arduino library: Adafruit MLX90614
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>    // Arduino library: Adafruit SSD1306
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#include "luxmeter.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// define global variables
uint8_t gain = 1;
float lumens;
float tempC = 0;
float last_lumens_sent;
float last_tempC_sent;
unsigned long last_msg_time;
bool lux_sensor = false;
bool temp_sensor = false;
float divisor = 1.0;

void setupDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // set up the display
  display.clearDisplay();
  display.setRotation(2);  // flip display upsidedown
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi not connected.\nEntering AP mode,\nplease connect device\nto LuxMeter-AP to\nset up WiFi.");
  display.display();
}

void getConnected() {
  display.setCursor(0,0);
  display.println("Connecting to WiFi"); display.display();
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  //wm.resetSettings(); // reset settings - for testing
  wm.setAPCallback(configModeCallback);
  wm.autoConnect("LuxMeter-AP");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi connected.");
  display.println("IP Address: ");
  display.println(WiFi.localIP());
  display.display();
}

void setupSensors() {
  if (tsl.begin()) {
    lux_sensor = true;
    display.println("TSL2591 sensor found"); display.display();
  }
  else {
    display.println("TSL2591 not found!"); display.display();
  }

  gain = 1;
  tsl.setGain(TSL2591_GAIN_LOW);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);

  if (mlx.begin()) { // device found!
    temp_sensor = true;
    display.println("MLX90614 sensor found"); display.display();
  }
  else {
    display.println("MLX90614 not found!"); display.display();
  }

  delay(3000); // Pause for 3 seconds to display setup info
}

void set_divisor(float d) {
  EEPROM.put(0,d);
  EEPROM.commit();
  divisor = d;
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

void updateLumens() {
  uint16_t reading = tsl.getLuminosity(TSL2591_VISIBLE);
  
  if (reading == 65535) { return; } // bogus value, ignore and get out

  lumens = reading / divisor / gain;
  if (lumens >= 100) { lumens = round(lumens); } // round to 0 decimal points
  else { lumens = ((float)((int)(lumens * 10 + 0.5))) / 10; } // round to 1 decimal point

  // if we're less than 10 lumens, use 25x gain
  if (gain == 1 && lumens < 10) {
    gain = 25;
    tsl.setGain(TSL2591_GAIN_MED);
  }
  else if (gain > 1 && (lumens >= 10 || reading == 0)) {  // >=10 lumens or we're saturated
    gain = 1;
    tsl.setGain(TSL2591_GAIN_LOW);
  }
}

void updateTemp() {
  // updating every 100ms seems to be too much for the poor little sensor
  // so back off and only read it every 10th time (around 1s)
  static uint8_t temp_delay = 0;
  if (temp_delay == 0) {
    // not sure if this will help, but pause 10ms before and after the reading
    // to make sure the i2c bus is ready to go before calling the MLX90614.
    // it seems to be very sensitive and can get locked up on erroneous values
    delay(10);
    float reading = mlx.readObjectTempC(); // will return NAN if reading failed
    delay(10);
    Serial.println("Reading: " + String(reading,2));
    tempC = reading;
  }
  temp_delay++;
  if (temp_delay >= 10) { temp_delay = 0; }

}

void displayData() {
  int16_t  x1, y1;
  uint16_t w, h;

  display.clearDisplay();
  display.setFont(&FreeSansBold18pt7b);
  String lumens_str = String(lumens, (lumens <= 10 ? 1 : 0));
  display.getTextBounds(lumens_str, 0, 28, &x1, &y1, &w, &h);
  display.setCursor(display.width() - w - 5, 28);
  display.println(lumens_str);
  if (temp_sensor) {
    if(tempC < 16 || tempC > 30) Serial.println("Temperature: " + String(tempC) + "*C");
    display.setCursor(0,63);
    display.print(tempC,0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(display.getCursorX(), 52);
    display.print(" *C");
  }
  display.setFont(&FreeSansBold9pt7b);
  display.getTextBounds((String)divisor, 0, 63, &x1, &y1, &w, &h);
  display.setCursor(display.width() - w - 5, 63);
  display.print((String)divisor);
  display.display();
}

void notifyClients(uint32_t client_id = 0) {
  String msg = String(lumens, (lumens <= 100 ? 1 : 0)) + ", " + String(tempC,1);
  if(client_id == 0) { ws.textAll(msg); }
  else { ws.text(client_id, msg); }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t client_id) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String str = String((char*)data);
    if(str.startsWith("divisor:")) {
      float d = str.substring(1+str.indexOf(":")).toFloat();
      set_divisor(d);
    }
    if(str == "update") { // got a request for an update
      notifyClients(client_id);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len, client->id());
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  if(var == "LUMENS"){ return String(lumens, (lumens <= 100 ? 1 : 0)); }
  if(var == "TEMP"){ return String(tempC,1); }
}

void setup() {
  Serial.begin(115200);

  EEPROM.begin(4);  // a float needs 4 bytes
  divisor = get_divisor();

  setupDisplay();
  getConnected();
  setupSensors();

  delay(3000); // wait so user can see setup info

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // and set up one for the favicon
  server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "image/svg+xml", favicon_svg);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();
  if(lux_sensor) { updateLumens(); }
  if(temp_sensor) { updateTemp(); }
  displayData();

  // only notify if lumens or temp has changed significantly
  // or if it has been more than a minute since the last msg
  bool send_update = 0;
  
  if(last_lumens_sent == 0) {
    if(lumens != 0) { send_update = 1; }
  }
  else { // not zero, don't worry about DIV0 problems
    if(lumens >= 100 && abs((last_lumens_sent-lumens)*100/last_lumens_sent) >= 2) { send_update = 1; }
    if(lumens < 100 && abs(last_lumens_sent-lumens) >= 2) { send_update = 1; }
    if(lumens < 10 && abs(10*(last_lumens_sent-lumens)) >= 2) { send_update = 1; }
  }

  if(last_tempC_sent == 0) {
    if(tempC != 0) { send_update = 1; }
  }
  else { // not zero, don't worry about DIV0 problems... if > 2% change AND > 0.3 degree change
    if(abs((last_tempC_sent-tempC)*100/last_tempC_sent) >= 2 && abs(last_tempC_sent-tempC)*10 >= 3) { send_update = 1; }
  }

  if(millis() - last_msg_time > 60000) {
    send_update = 1;
  }
  if(send_update) { 
    last_lumens_sent = lumens;
    last_tempC_sent = tempC;
    last_msg_time = millis();
    notifyClients();
  }
}
