/**
   Send DHT22 Sensor Data to EmonCMS and go to deep sleep
   Philipp Schweizer - http://raspberry.tips
   08.10.2017 - Inital Version
*/

// Including the Libraries for WiFi Connection, HTTP Send and DHT22 Sensor
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "DHT.h"

// Replace with your network details
const char* ssid = "fritzbox";
const char* password = "xxxxxxxxxxxx";
//Provide IP Adresse, Gateway, Subnet and DNS matching your network
IPAddress ip(192, 168, 178, 211); // I use fixed IP-Adress
IPAddress gateway(192, 168, 178, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your
IPAddress dns(192, 168, 178, 1); //DNS Server = Router IP

// DHT Sensor settings
#define DHTTYPE DHT22
const int DHTPin = 5;
DHT dht(DHTPin, DHTTYPE);

//emoncoms settings, change node ID for every node
const char* host = "xyz.de";
const char* nodeId   = "31";
const char* privateKey = "123123123123123123123";
const int httpPort = 80;

//Seconds to deep sleep 300 = 5Min
const int sleepTimeInSec = 300;

//Variables to store values
float h = 0;
float t = 0;
int vcc;
ADC_MODE(ADC_VCC);

// ==============================================
// only runs once on boot then go deep sleep again
// ==============================================
void setup() {
  
  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  delay(500);

  //Start sensor
  dht.begin();

  //Connect WiFi
  //Fix  https://github.com/esp8266/Arduino/issues/2186
  WiFi.config(ip, gateway, subnet, dns);
  delay(100);
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Sensor readings may also be up to 2 seconds 'old', so we read twice
  h = dht.readHumidity();
  t = dht.readTemperature();
  delay(2500);
  h = dht.readHumidity();
  t = dht.readTemperature();
 
  vcc = ESP.getVcc();//readvdd33();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  
  //Send data to emoncms
  sendData();

  //go to deep sleep
  ESP.deepSleep(sleepTimeInSec * 1000000);
}

// ==============================================
// Send Data to emoncms
// ==============================================
void sendData() {

  WiFiClient emonHTTPClient;

  //Connect by http
  const int httpPort = 80;
  if (!emonHTTPClient.connect(host, httpPort)) {
    return;
  }

  //Build the JSON for emoncms to send data
  String json = "{temperature:";
  json += t;
  json += ",humidity:";
  json += h;  
  json += ",vcc:";
  json += vcc;
  json += "}";

  //Build emoncms URL for sending data to
  String url = "/input/post.json?node=";
  url += nodeId;
  url += "&apikey=";
  url += privateKey;
  url += "&json=";
  url += json;

  // Send the HTTP Request to the emoncms server
  emonHTTPClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Connection: close\r\n\r\n");
  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (emonHTTPClient.available()) {
    String line = emonHTTPClient.readStringUntil('\r');
  }

  emonHTTPClient.stop();
}



void loop(void) {
}   

