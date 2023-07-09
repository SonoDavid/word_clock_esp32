#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266wifi.h>

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // // print your board's IP address:
  // IPAddress ip = WiFi.localIP();
  // Serial.print("To see this page in action, open a browser to http://");
  // Serial.println(ip);

  // print the received signal strength:

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}