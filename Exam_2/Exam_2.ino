/*********************************************************
  UPB - Internet of Things
  2022 - 10
  Kevin Alejandro Vasco Hurtado

  Sensors:
    - Temperature and Humidity:  HDC 1080
    - Lux: AP3216
    - GPS: L70 BEE
**********************************************************/

#include <Wire.h>
#include <ESP8266WiFi.h>          //  Library for WiFi
#include <ESP8266WiFiMulti.h>     //  Library for 
#include <ClosedCube_HDC1080.h>   //  Library for HDC 1080
#include <AP3216_WE.h>            //  Library for AP3216
#include <TinyGPSPlus.h>          //  Library for L70 BEE
#include <SoftwareSerial.h>       //  Library for Serial Ports
#include <Hash.h>               //  Library for Hash

/*
 * Defining variables for WiFi connection
*/
#ifndef STASSID
#define STASSID "TIGO-2A72" //"your-ssid"
#define STAPSK "1000404305Nc7TQx" //"your_password"
#endif

ClosedCube_HDC1080 temp_hum_sensor;
AP3216_WE lux_sensor = AP3216_WE();
TinyGPSPlus gps;
SoftwareSerial ss(13, 15);        //  Using Pin 13 for Transmision (Tx) and Pin 15 for Reception (Rx)
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;

static const int GPSBaud = 9600;  //  Frecuency for data exchange rate between GPS and Arduino
static const int iter = 15;       //  Amount of iterations for Data Collection
static const int id = 363699;     //  Id for Datagram
const char* ssid = STASSID;       //  WLAN name
const char* password = STAPSK;    //  WLAN password
const char* host = "34.207.122.235";            //  To whom I'm connecting
const uint16_t port = 80;        //  Port for connection // 80 443

String epochtime, timestamp, lat_gps, long_gps, alt_gps, temp, humidity, lux, data, encryption ,datagram;
int sec = 0;                      //  Sequence

void setup() {
  Serial.begin(115200);           //  Data exchange rate between Arduino and Serial Monitor // 115200 9600

  /*
    L70 BEE Setup
  */
  ss.begin(GPSBaud);              //  Data exchange rate between GPS and Arduino

  /*
    HDC 1080 Setup
  */
  temp_hum_sensor.begin(0x40);    //  14 bit Temperature and Humidity Measurement Resolutions

  /*
    AP3216 Setup
  */
  Wire.begin();
  lux_sensor.init();
  lux_sensor.setMode(AP3216_ALS);           //  Continuos Ambient Light data collection
  lux_sensor.setLuxRange(RANGE_20661);      //  Resolution
  //lux_sensor.setALSCalibrationFactor(1.25); //  Calibration for Ambient Light in case of light being abstructed. E.g. if an ALS value is 80% behind a window then select 100/80 = 1.25 as calibration factor.

  /*
   * WiFi Conection Setup
  */
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
  while (WiFiMulti.run() != WL_CONNECTED) {}
}

/*
  State Machine
*/
void loop() {
  sec++;
  Serial.println("In GPS");
  SMART_DELAY(15000);
  Serial.println("In tmp/hum");
  TEMP_HUMIDITY_COLLECTION();
  Serial.println("In lux");
  LUX_COLLECTION();
  Serial.println("Bonding");
  BONDING();
  Serial.println(datagram);
  Serial.println("Sending");
  SENDING();
  Serial.println();
}

/*
   Data Collection for L70 BEE
*/
static void GPS_COLLECTION() {
  bool new_data_gps = false;
  char c = ss.read();
  //Serial.write(c);
  if (gps.encode(c)) // Did a new valid sentence come in?
    new_data_gps = true;

  if (new_data_gps) {
    lat_gps = String(gps.location.lat());
    long_gps = String(gps.location.lng());
    alt_gps = String(gps.altitude.meters());

    /*
       Building timestamp for package
    */
    timestamp = "";
    timestamp.concat(gps.date.day());
    timestamp.concat("/");
    timestamp.concat(gps.date.month());
    timestamp.concat("/");
    timestamp.concat(gps.date.year());
    timestamp.concat("_");
    timestamp.concat(gps.time.hour());
    timestamp.concat(":");
    timestamp.concat(gps.time.minute());
    timestamp.concat(":");
    timestamp.concat(gps.time.second());
  }
}

/*
   Smart Delay for GPS
*/
static void SMART_DELAY(unsigned long ms) {
  unsigned long start = millis();
  do
  {
    while (ss.available())
      GPS_COLLECTION();
  } while (millis() - start < ms);
}

/*
   Data Collection for HDC 1080 Sensor
*/
static void TEMP_HUMIDITY_COLLECTION() {
  double temp_Arr[iter];  // Array of Temperature data
  double hum_Arr[iter];   // Array of Humidity data

  for (int i = 0; i < iter; i++) {
    temp_Arr[i] = temp_hum_sensor.readTemperature();
    hum_Arr[i] = temp_hum_sensor.readHumidity();
  }

  temp = PRUNING(temp_Arr);
  humidity = PRUNING(hum_Arr);
  SMART_DELAY(0);
}

/*
   Data Collection for AP3216 Sensor
*/
static void LUX_COLLECTION() {
  double lux_Arr[iter];    // Array of Lux data

  for (int i = 0; i < iter; i++) {
    lux_Arr[i] = lux_sensor.getAmbientLight();
  }

  lux = PRUNING(lux_Arr);
  SMART_DELAY(0);
}

/*
   Pruning function for HDC 1080 & AP3216
*/
String PRUNING(double arr[]) {
  double value;

  for (int i = 0; i < iter; i++) {
    value += arr[i];
  }

  return String(value / iter);
}

static void BONDING() {
  data = "\n\t{\"sequence\":"          + String(sec) + ","  +
         "\n\t\"identifier\":"         + String(id)  + ","  +
         "\n\t\"timestamp\":" + "\""   + timestamp   + "\"" + "," +
         "\n\t\"latitud\":"            + lat_gps     + ","  +
         "\n\t\"longitud\":"           + long_gps    + ","  +
         "\n\t\"altitud\":"            + alt_gps     + ","  +
         "\n\t\"temperature\":"        + temp        + ","  +
         "\n\t\"humidity\":"           + humidity    + ","  +
         "\n\t\"lux\":"                + lux         + "\n\t}";

  encryption = String(sha1(data));
  datagram = "{\n\"data\":"            + data        + ","  +
             "\n\"checksum\":" + "\""  + encryption  + "\"" + "," +
             "\n\"signature\":" + "\"" + "Sign"      + "\"" + "\n}"; 
}

static void SENDING() {
  if (client.connect(host, port)) {
    client.println("POST / HTTP/1.1"); // /data is a folder within the host. Create a host and the folder
    client.print("HOST: "); client.println(host);
    client.println("User-Agent: ESP8266");  // User-Agent: Arduino/1.0
    client.println("Connection: close");
    client.println("Content-Type: application/json;");
    client.print("Content-Length: "); client.println(datagram.length());
    client.println();
    client.println(datagram);
  }
}
