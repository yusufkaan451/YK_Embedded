

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <Update.h>


const char* ssid = "ayy";
const char* password = "w!123456";

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println("Connected to Wi-Fi.");
  Serial.println(WiFi.localIP());

  // OTA Update Endpoint
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Endpoint for when the update completes
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    request->send(response);
    ESP.restart();
  }, [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    // Handle firmware upload
    if (!index) {
      Serial.printf("Update Start: %s\n", filename.c_str());
      Update.begin(UPDATE_SIZE_UNKNOWN);
    }

    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }

    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello, world. Go to /update to update firmware.");
  });

  server.begin();
}

void loop() {
  // Handle background tasks
}