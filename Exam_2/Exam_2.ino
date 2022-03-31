//Copyright (c) {{ 2022 }} {{ UPB - Internet of Things - Kevin Alejandro Vasco Hurtado }}

//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:

//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
//OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
//OR OTHER DEALINGS IN THE SOFTWARE.

/***************************************
  Microchip:
    - NodeMCU ESP8266
  Sensors:
    - Lux: AP3216
    - GPS: L70 BEE
    - Temperature and Humidity: HDC 1080
****************************************/

#include <ESP8266WiFi.h>                    //  Library for WiFi Connection
#include <AP3216_WE.h>                      //  Library for AP3216
#include <TinyGPSPlus.h>                    //  Library for L70 BEE
#include <SoftwareSerial.h>                 //  Library for Serial Ports
#include <ESP8266WiFiMulti.h>               //  Library for WiFi Connection
#include <ClosedCube_HDC1080.h>             //  Library for HDC 1080
#include <Hash.h>                           //  Library for Hash
#include <ESP8266HTTPClient.h>              //  Library for HTTO
#include <Arduino.h>                        //  Library for Arduino SDK
#include <ArduinoJson.h>                    //  Library for Easy Json Creation
#include <WiFiClientSecureBearSSL.h>        //  Library for validation of HTTPS Certificate

TinyGPSPlus gps;
SoftwareSerial ss(13, 15);                  //  Using Pin 13 for Transmision (Tx) and Pin 15 for Reception (Rx)
ESP8266WiFiMulti WiFiMulti;
ClosedCube_HDC1080 temp_hum_sensor;
AP3216_WE lux_sensor          = AP3216_WE();

const char* ID                = "363699";
const char* STASSID           = "UPBWiFi";
//const char* STASSID           = "TIGO-2A72";                          //  Your SSID
const char* STAPSK            = "";
//const char* STAPSK            = "1000404305Nc7TQx";                   //  Your password
const char* SERVER_IP         = "https://44.203.114.71/sensor_data"; //  Complete route
//  Fingerprint of your SSL Certificate.
const uint8_t fingerprint[20] = {0xD3, 0x5D, 0xFE, 0xF0, 0xAF, 0x61, 0xC1, 0x66, 0x9B, 0x05, 0x53, 0x3B, 0xBA, 0x44, 0x72, 0x93, 0x05, 0xE9, 0xF5, 0x18};
const uint16_t GPSBaud        = 9600;                                 //  Frecuency for data exchange rate between GPS and Arduino
const uint16_t ITERATIONS     = 15;                                   //  Amount of iterations for Data Collection

String TIMESTAMP, LATITUD, LONGITUD, ALTITUD, TEMPERATURE, HUMIDITY, LUX, PARTIAL_DATAGRAM, DATAGRAM;
int SEQUENCE;

//  Smart Delay for incoming GPS data
static void SMART_DELAY(unsigned long ms) {
  unsigned long start = millis();
  do {
    //  Waiting for Transmission and Reception Channels to be available
    while (ss.available())
      GPS_COLLECTION();
  } while (millis() - start < ms);
}

//  Data Collection for L70 BEE
static void GPS_COLLECTION() {

  bool new_gps_data = false;
  //  Reading incoming sentence
  char c = ss.read();
  // Did a new valid sentence come in?
  if (gps.encode(c))
    new_gps_data = true;

  //  Reading and converting NMEA
  if (new_gps_data) {
    LATITUD  = String(gps.location.lat());
    LONGITUD = String(gps.location.lng());
    ALTITUD  = String(gps.altitude.meters());

    //  Building TIMESTAMP for package in SQL format
    TIMESTAMP = "";
    TIMESTAMP.concat(gps.date.year());
    TIMESTAMP.concat("/");
    TIMESTAMP.concat(gps.date.month()  > 9 ? String(gps.date.month())  : "0" + String(gps.date.month()));
    TIMESTAMP.concat("/");
    TIMESTAMP.concat(gps.date.day()    > 9 ? String(gps.date.day())    : "0" + String(gps.date.day()));
    TIMESTAMP.concat("_");
    TIMESTAMP.concat(gps.time.hour()   > 9 ? String(gps.time.hour())   : "0" + String(gps.time.hour()));
    TIMESTAMP.concat(":");
    TIMESTAMP.concat(gps.time.minute() > 9 ? String(gps.time.minute()) : "0" + String(gps.time.minute()));
    TIMESTAMP.concat(":");
    TIMESTAMP.concat(gps.time.second() > 9 ? String(gps.time.second()) : "0" + String(gps.time.second()));
  }
}

//  Data Collection for HDC 1080 Sensor
static void TEMP_HUMIDITY_COLLECTION() {

  double hum_Arr[ITERATIONS];
  double temp_Arr[ITERATIONS];

  //  Reading temperature and humidity values
  for (int i = 0; i < ITERATIONS; i++) {
    hum_Arr[i]  = temp_hum_sensor.readHumidity();
    temp_Arr[i] = temp_hum_sensor.readTemperature();
  }
  //  Pruning obtained values
  HUMIDITY    = PRUNING(hum_Arr);
  TEMPERATURE = PRUNING(temp_Arr);
  SMART_DELAY(0);
}

//  Data Collection for AP3216 Sensor
static void LUX_COLLECTION() {

  double lux_Arr[ITERATIONS];
  //  Reading lux values
  for (int i = 0; i < ITERATIONS; i++) {
    lux_Arr[i] = lux_sensor.getAmbientLight();
  }
  //  Pruning obtained values
  LUX = PRUNING(lux_Arr);
  SMART_DELAY(0);
}

//  Pruning function for HDC 1080 & AP3216
String PRUNING(double arr[]) {
  double value;
  for (int i = 0; i < ITERATIONS; i++) {
    value += arr[i];
  }
  return String( value / ITERATIONS );
}

//  Construction of body for POST
void BUNDLING() {

  TIMESTAMP = "2022/03/29_01:42:51";
  LATITUD = "6.18";
  LONGITUD = "-75.61";
  ALTITUD = "750.80";
  TEMPERATURE = "24.56";
  HUMIDITY = "77.72";
  LUX = "16.80";
  
  DATAGRAM = "";
  PARTIAL_DATAGRAM = "";
  DynamicJsonDocument doc_1(1024);
  DynamicJsonDocument doc_2(1024);
  
  JsonObject data     = doc_1.to<JsonObject>();
  data["identifier"]  = ID;
  data["sequence"]    = String(SEQUENCE);
  data["TIMESTAMP"]   = TIMESTAMP;
  data["latitud"]     = LATITUD;
  data["longitud"]    = LONGITUD;
  data["altitud"]     = ALTITUD;
  data["temperature"] = TEMPERATURE;
  data["humidity"]    = HUMIDITY;
  data["lux"]         = LUX;

  serializeJson(doc_1, PARTIAL_DATAGRAM);

  JsonObject root     = doc_2.to<JsonObject>();
  root["data"]        = PARTIAL_DATAGRAM;
  root["checksum"]    = "CHECKSUM";
  root["signature"]   = "sign";

  serializeJson(doc_2, DATAGRAM);
}

void SENDING() {
  //  Wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(fingerprint);
    //  Or, if you happy to ignore the SSL certificate, then use the following line instead:
    //  client->setInsecure();

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    https.begin(*client, SERVER_IP);
    https.addHeader("Content-Type", "application/json");
      
    Serial.print("[HTTPS] POST...\n");
    //  Start connection and send HTTP header
    int httpCode = https.POST(DATAGRAM);
    //  HttpCode will be negative on error
    if (httpCode > 0) {
      //  HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  //Serial.println("Wait 10s before next round...");
  //delay(10000);
}
void setup() {

  Serial.begin(115200); //  Data exchange rate between Arduino and Serial Monitor

  // Waiting for any outgoing data to send
  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  
  //  WiFi Conection Setup
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);

  //  L70 BEE Setup
  ss.begin(GPSBaud);  //  Data exchange rate between GPS and Arduino

  //  HDC 1080 Setup
  temp_hum_sensor.begin(0x40);  //  14 bit Temperature and Humidity Measurement Resolutions

  //  AP3216 Setup
  Wire.begin();
  lux_sensor.init();
  lux_sensor.setMode(AP3216_ALS); //  Continuos Ambient Light data collection
  lux_sensor.setLuxRange(RANGE_20661);  //  Resolution
  //lux_sensor.setALSCalibrationFactor(1.25); //  Calibration for Ambient Light in case of light being abstructed. E.g. if an ALS value is 80% behind a window then select 100/80 = 1.25 as calibration factor.
}

void loop() {
  SEQUENCE++;
  //SMART_DELAY(15000);
  //TEMP_HUMIDITY_COLLECTION();
  //LUX_COLLECTION();
  BUNDLING();
  SENDING();
}
