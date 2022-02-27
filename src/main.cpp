#include <Arduino.h>

// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
// Load Wi-Fi library
#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

#define FU_HTTPS

//Your Domain name with URL path or IP address with path
String serverName = "https://192.168.1.7/test_get.php";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;


#define DHTPIN 22     // Digital pin connected to the DHT sensor 
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);

// Replace the following ca certificate with your server's certificate
#ifdef FU_HTTPS
const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDmzCCAoMCFEGekyA+W02NLVwtBzTiy6kLbrFHMA0GCSqGSIb3DQEBCwUAMIGJ\n" \
"MQswCQYDVQQGEwJVUzEWMBQGA1UECAwNTWFzc2FjaHVzZXR0czEPMA0GA1UEBwwG\n" \
"TG93ZWxsMQwwCgYDVQQKDANVTUwxCzAJBgNVBAsMAkNTMRQwEgYDVQQDDAsxOTIu\n" \
"MTY4LjEuNzEgMB4GCSqGSIb3DQEJARYReGlud2VuX2Z1QHVtbC5lZHUwHhcNMjIw\n" \
"MjI3MDIyNTA5WhcNMjMwMjI3MDIyNTA5WjCBiTELMAkGA1UEBhMCVVMxFjAUBgNV\n" \
"BAgMDU1hc3NhY2h1c2V0dHMxDzANBgNVBAcMBkxvd2VsbDEMMAoGA1UECgwDVU1M\n" \
"MQswCQYDVQQLDAJDUzEUMBIGA1UEAwwLMTkyLjE2OC4xLjcxIDAeBgkqhkiG9w0B\n" \
"CQEWEXhpbndlbl9mdUB1bWwuZWR1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n" \
"CgKCAQEAuOhoDm1+zc6XnyhWayfpRkxTbsSqML7OE1NkzCG1WeX686WYKEoPyObG\n" \
"vLBPFO7X2iu3m/pWP/mruWO5+7yTFIPi/2nDRbEngbHadl/4YiABTHfw9FuHkLWL\n" \
"1Et8h/yl1qo0waNHGFluBdsFu09rRCDZbi0o8Pkt+AsoFZSOg0zOxp7/OHs6muPP\n" \
"+LlCMxHvgY8sh6l3wjDRd/YCJceLRB+hhBdn6rPpXK5/00jNrT8r928I9fOUVR+h\n" \
"yDHluG8AMP/iTsi2UoTp1j9gD0WCEqwnVtzXGeeak8ef3yQ3jSwvxXMNK4qoba9W\n" \
"YHyhAJ/v2OjCJP0CCDxEbEJj2n7dQQIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQAY\n" \
"qGFBk4kpUJc49en/bsMBpHZHS8zVPThj8NyZ1OEtN4uLGmNdj1XERn2JE8CDtPyn\n" \
"6Dqpy+TOJl78niYI5zreUYkBhEJfsZmr2OQXc8FP9ht3emQAqTyuxzDgb+BkQMzA\n" \
"etAqJ1t3SQqOqVJ+6nPA06nXNgmgfsSW8XDYnxmp3bDsTiY9G2qPwO6TEhYoI5Tj\n" \
"MzUTQh5M10gPKJHL5qlvpuEkSvh5OIjsIlftgY9bVFxqWvMLZyHwZTOHYE4QJ5PR\n" \
"mdfcQAjZPxe0ItQiiSsrOIVqiKEKZW6/IMb28bqBLRiHsw3eKvho2gF7oR1QsB3N\n" \
"Tgx2ehqfvmu2dxJ/+FWr\n" \
"-----END CERTIFICATE-----\n";
#endif

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      sensors_event_t event;
      dht.temperature().getEvent(&event);
      if (isnan(event.temperature)) {
        Serial.println(F("Error reading temperature!"));
      }
      else {
        Serial.print(F("Temperature: "));
        Serial.print(event.temperature);
        Serial.println(F("°C"));
      }

      // Display current state, and ON/OFF buttons for GPIO 26  
      String myTemperatureString(event.temperature);     // empty string

      dht.humidity().getEvent(&event);
       if (isnan(event.relative_humidity)) {
        Serial.println(F("Error reading humidity!"));
      }
      else {
        Serial.print(F("Humidity: "));
        Serial.print(event.relative_humidity);
        Serial.println(F("%"));
      }

      String myHumidityString(event.relative_humidity);     // empty string

      String serverPath = serverName + "?Temperature="+myTemperatureString+"&Humidity="+myHumidityString;
     
      #ifdef FU_HTTP
      http.begin(serverPath.c_str());
      #endif

      // Your Domain name with URL path or IP address with path
      #ifdef FU_HTTPS
      http.begin(serverPath.c_str(), root_ca);
      #endif

      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
