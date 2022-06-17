#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

#define CHECK_FOR_UPDATES_INTERVAL 5
#ifndef VERSION
  #define VERSION "0.0.9"
#endif

#ifndef REPO_URL
  #define REPO_URL "elliotmatson/GHA-OTA-test"
#endif

unsigned long getUptimeSeconds();
void firmwareUpdate();
void checkForUpdates(void * parameter);

TaskHandle_t checkForUpdatesTask = NULL;


void setup() {
  Serial.begin(115200);
  Serial.print("Booting: ");
#ifdef VERSION
  Serial.println(VERSION);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(2, OUTPUT);

  xTaskCreate(
    checkForUpdates,    // Function that should be called
    "Check For Updates",   // Name of the task (for debugging)
    6000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    0,               // Task priority
    &checkForUpdatesTask             // Task handle
  );
}

void loop() {
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(100);
}

void checkForUpdates(void * parameter){
  for(;;){
    firmwareUpdate();
    vTaskDelay((CHECK_FOR_UPDATES_INTERVAL*1000) / portTICK_PERIOD_MS);
  }
}

void firmwareUpdate()
{
#ifdef VERSION
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();

    String firmwareUrl = String("https://github.com/") + REPO_URL + String("/releases/latest/download/esp32.bin");
    Serial.println(firmwareUrl);
    
    if (!http.begin(client, firmwareUrl))
        return;

    int httpCode = http.sendRequest("HEAD");
    if (httpCode < 300 || httpCode > 400 || http.getLocation().indexOf(String(VERSION)) > 0)
    {
        Serial.printf("Not updating from (sc=%d): %s\n", httpCode, http.getLocation().c_str());
        http.end();
        return;
    }
    else
    {
        Serial.printf("Updating from (sc=%d): %s\n", httpCode, http.getLocation().c_str());
    }

    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("Http Update Failed (Error=%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.printf("No Update!\n");
        break;

    case HTTP_UPDATE_OK:
        Serial.printf("Update OK!\n");
        break;
    }
#endif
}

unsigned long getUptimeSeconds()
{
    return (unsigned long)(esp_timer_get_time() / 1000000ULL);
}