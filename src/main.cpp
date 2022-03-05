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
const char* ssid = "CWS5Q";
const char* password = "LN7S3S282Q7NLCD8";

#define FU_HTTPS

//Your Domain name with URL path or IP address with path
#ifdef FU_HTTP
String serverName = "http://192.168.1.7/test_get.php";
#endif

#ifdef FU_HTTPS
String serverName = "https://192.168.1.7/test_get.php";
#endif

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

#ifdef FU_HTTPS
const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIID3TCCAsWgAwIBAgIUMPGWkElQF+85oOxgDJ/hvse44CkwDQYJKoZIhvcNAQEL\n" \
"BQAwfjELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk1BMQ8wDQYDVQQHDAZMb3dlbGwx\n" \
"DDAKBgNVBAoMA1VNTDELMAkGA1UECwwCQ1MxFDASBgNVBAMMCzE5Mi4xNjguMS43\n" \
"MSAwHgYJKoZIhvcNAQkBFhF4aW53ZW5fZnVAdW1sLmVkdTAeFw0yMjAyMjcwNTU4\n" \
"MjdaFw0yMzAyMjcwNTU4MjdaMH4xCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJNQTEP\n" \
"MA0GA1UEBwwGTG93ZWxsMQwwCgYDVQQKDANVTUwxCzAJBgNVBAsMAkNTMRQwEgYD\n" \
"VQQDDAsxOTIuMTY4LjEuNzEgMB4GCSqGSIb3DQEJARYReGlud2VuX2Z1QHVtbC5l\n" \
"ZHUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDyvCLMXves9I+D6rOa\n" \
"7Fc6tydf1SwMnQU9f2Iwsd2G/NallJ0TdFkIXZJjHcNleE87DldYvhX/jn/dtLLh\n" \
"Np9SY9Mr9M/vHNewJYBREYNgVcTghyf5BTGLTpnatLER6/Q8hEsY377fNlpeR2Um\n" \
"v92go0YgZJ8Vgg/P8OA6tNevI9BTkXSI5MkLlmbcLm0wy1ryUwOvLb7uJ50jq02z\n" \
"1c1EhyoLMgD7iNHi9LiW3hr4VVIxJ6hoyllo9FMqo4CUInjecBTCQByKT3sR8KWh\n" \
"Q67yG0ZlFaBDEwufUT0ny+gJaWe6GmeKuqDldt7rYbQD/Pi7Yg0U4qYoT7ry4MiB\n" \
"7M/9AgMBAAGjUzBRMB0GA1UdDgQWBBTlUha/ZZDKTO478xxQD8Jyq3SznTAfBgNV\n" \
"HSMEGDAWgBTlUha/ZZDKTO478xxQD8Jyq3SznTAPBgNVHRMBAf8EBTADAQH/MA0G\n" \
"CSqGSIb3DQEBCwUAA4IBAQAWGPnAbx+1qNLBszlVF5eX2VbrfLb7doKpWPKB/fG3\n" \
"icXo7ry2+b4+TgSG7T3hIFy9Vfmr9Y9dYpsxL/6kKg5C8xKo9VhfwNipyBe+jsIM\n" \
"ytH52lcNoNFaIxbg8IaqP47ZgsTFpiX7Ni59CgAw/ZkIG3zEZdXCixSlfA7j0Vny\n" \
"tGqHD65flS7cY9O3jPbberboMAcMz9XQl2H3qRgqP7KlriV2oOvy4Av4PkU3ZWTp\n" \
"RJ4/GVBPki5+3hgw7aRbsETFr0aAsYp+5o6fzRhXOoPQHIfZke0DTbL/LkPELg0w\n" \
"AE0H8i/pbfhL0FPwvgsYZ+6nBGjs/hmmU5UCDWyGwayG\n" \
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
        Serial.print("httpResponseCode (Error) code: ");
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