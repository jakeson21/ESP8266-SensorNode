/*
 * See: https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/using-the-arduino-addon
 * https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino#L71
 */

#include <ESP8266WiFi.h>

/* 
 * "WifiSetup.h" contains the following 4 globals, with appropriate values
 * const char* ssid     = "****";
 * const char* password = "****";
 * const char* host     = "192.168.0.1";
 * const int   port     = 1234;
*/
#include "WifiSetup.h"
#include "SensorNodeEnums.h"
const String DeviceId = String(BEDROOM1);

// Use WiFiClient class to create TCP connections
WiFiClient client;

// DHT type sensor
#include "DHT.h"
#define DHTPIN 5     // what digital pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

void setup() 
{
    // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("Welcome");
  Serial.println();
  delay(100);
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFiUp();
  WiFi.setOutputPower(9.0);
  Serial.print("WiFi Connected [");
  long rssi_dBm = WiFi.RSSI();
  Serial.print(rssi_dBm);
  Serial.println(" dBm]");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  digitalWrite(DHTPIN, HIGH);
  delay(100);
  digitalWrite(DHTPIN, LOW);
  delay(100);
  digitalWrite(DHTPIN, HIGH);
}

void loop() 
{
//  WiFiUp();
  if (!ConnectToHost()) { return; }
  long rssi_dBm = WiFi.RSSI();

  /// Take Reading
  float TempF, Humidity, HeatIndexF;
  if(getSensorReading(TempF, Humidity, HeatIndexF))
  {
    String jsonData = "{ \"temperature\": " + String(TempF, 2);
    jsonData += ", \"temperature_units\": \"F\", \"deviceId\" : " + DeviceId;
    jsonData += ", \"humidity_units\": \"%\", \"humidity\": " + String(Humidity, 2);
    jsonData += ", \"heatindex_units\": \"F\", \"heatindex\": " + String(HeatIndexF, 2);
    jsonData += ", \"rssi_units\": \"dBm\", \"rssi\": " + String(rssi_dBm);
    jsonData += " }";
    Serial.print("Sending: ");
    Serial.println(jsonData);
   
    /// Send Reading
    // This will send the request to the server
    client.println(jsonData);
  }

  delay(1000);
  while(client.available()){
    String cmds = client.readStringUntil('\n');
    Serial.print("Recieved: ");
    Serial.println(cmds);
  }

//  Serial.println("Disconnecting");
//  client.stop();
  delay(100);
//  WiFiDown();
  delay(29000);
}

bool WiFiUp(void)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }
  
  WiFi.forceSleepWake();
  delay(100);  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);  
  WiFi.begin(ssid, password);
  int retry_count = 20;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry_count--;
    if (retry_count<=0) return false;
  }
  Serial.println();
  return true;
}

void WiFiDown()
{
  WiFi.disconnect();
  WiFi.forceSleepBegin();
  delay(100); //For some reason the modem won't go to sleep unless you do a delay(non-zero-number) -- no delay, no sleep and delay(0), no sleep - See more at: http://www.esp8266.com/viewtopic.php?p=38984#sthash.R0S3e0hR.dpuf
}

bool ConnectToHost()
{
  if (client.connected()) 
  { 
    Serial.println("Already connected");
    return true; 
    }

  Serial.print("Connecting to ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);
  if (!client.connect(host, port)) {
    Serial.println("Connection Failed");
    delay(1000);
    return false;
  }
  Serial.println("Connected");
  return true;
}

bool getSensorReading(float& outTempF, float& outHumidity, float& outHeatIndexF) {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return false;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  outHumidity = h;
  outTempF = f;
  outHeatIndexF = hif;
  
  return true;
  }
