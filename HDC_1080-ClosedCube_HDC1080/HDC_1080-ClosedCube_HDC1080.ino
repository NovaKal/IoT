/**************************************************************************************

This is example for ClosedCube HDC1080 Humidity and Temperature Sensor breakout booard

Initial Date: 13-May-2016

Hardware connections for Arduino Uno:
VDD to 3.3V DC
SCL to A5
SDA to A4
GND to common ground

Written by AA for ClosedCube

MIT License

**************************************************************************************/

#include <Wire.h>
#include <ClosedCube_HDC1080.h>

ClosedCube_HDC1080 sensor;

void setup() {
  Serial.begin(9600); // Data exchange rate between Arduino and Serial Monitor
  sensor.begin(0x40); // 14 bit Temperature and Humidity Measurement Resolutions
}

void loop() {
   /*
   * Data collection for HDC 1080 Sensor
   */
  unsigned int iter = 15; // Amount of iterations
  double temperature = 0;
  double humidity = 0;

  for (int i = 0; i < iter; i++) {
    temperature += sensor.readTemperature();
    humidity += sensor.readHumidity();
  }

   /*
   * Prunning of data
   */
  temperature /= iter;
  humidity /= iter;

   /*
   * Visualization of data collected
   */
  Serial.print("Temperature = "); Serial.print(temperature); Serial.println(" C");
  Serial.print(" Humidity = "); Serial.print(humidity); Serial.println("%");
  
  delay(1000);
}
