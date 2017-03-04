/*
 * See: https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/using-the-arduino-addon
 * https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino#L71
 */

#define ESP8266_LED 5

#include <ESP8266WiFi.h>

/* 
 * "WifiSetup.h" contains the following 4 globals, with appropriate values
 * const char* ssid     = "****";
 * const char* password = "****";
 * const char* host     = "192.168.0.1";
 * const int sensorPort = 1234;
*/
#include "WifiSetup.h"
#include "SensorNodeEnums.h"
const String DeviceId = String(ATTIC);

// Use WiFiClient class to create TCP connections
WiFiClient client;

#include <Wire.h> 
#include <DS1621.h>
byte sensoraddr = (0x90 >> 1) | 0;  // replace the 0 with the value you set on pins A2, A1 and A0
DS1621 sensor = DS1621(sensoraddr);


void setup() 
{
    // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("Welcome");

  pinMode(ESP8266_LED, OUTPUT);

  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!client.connect(host, sensorPort)) {
    Serial.println("connection failed");
    return;
  }
  Serial.print("Connected to: ");
  Serial.print(host);
  Serial.print(" ");
  Serial.println(sensorPort);

  /// Configure for DS1621 temperature sensor
  // Set A2,A1,A0 = FALSE  
  // Connect to any 3 GND pins
  // Wire.begin([SDA], [SCL]) will assume pins 2 and 14 are SDA and SCL
  Wire.begin(4, 5);
  sensor.startConversion(false);                       // stop if presently set to continuous
  sensor.setConfig(DS1621::POL | DS1621::ONE_SHOT);                    // Tout = active high; 1-shot mode
  sensor.setThresh(DS1621::ACCESS_TH, 27);                     // high temp threshold = 80F
  sensor.setThresh(DS1621::ACCESS_TL, 18);                     // low temp threshold = 65F
}

void loop() 
{
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();

    Serial.println("reconnecting.");
    if (!client.connect(host, sensorPort)) {
      Serial.println("connection failed");
      delay(1000);
      return;
    }
    Serial.println("connected");
  }
  
  digitalWrite(ESP8266_LED, HIGH);
  delay(500);
  digitalWrite(ESP8266_LED, LOW);
  delay(500);

  /// Take Reading
  long reading = 0;
  reading = getSensorReading();
  float degreesF = Celcius2Fahrenheit(reading * 0.01);

  String jsonData = "{ \"value\": " + String(degreesF, 2);
  jsonData += ", \"units\": \"F\", \"type\": \"temperature\", \"deviceId\" : " + DeviceId + " }";
  Serial.print("Sending: ");
  Serial.println(jsonData);

  /// Send Reading
  // This will send the request to the server
  client.println(jsonData);

  while(client.available()){
    String cmds = client.readStringUntil('\n');
    Serial.print("Recieved: ");
    Serial.println(cmds);
  }

  delay(5000);
}

int getSensorReading(void) {
  return sensor.getHrTemp();                             // read high-resolution temperature
}

float Celcius2Fahrenheit(float celsius)
{
  return 1.8 * celsius + 32;
}
