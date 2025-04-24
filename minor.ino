#include <WiFi.h>
#include <Arduino.h>
#include <ESP32QRCodeReader.h>

// WiFi credentials
const char* ssid     = "real"; // Change this to your WiFi SSID
const char* password = "Sangu@2003"; // Change this to your WiFi password

// ThingSpeak settings
const char* host = "api.thingspeak.com";
const int httpPort = 80;
const String channelID = "2232791"; // Change this to your channel ID
const String writeApiKey = "5J4ATLHBNPXSQ672"; // Change this to your Write API key

// QR code reading
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
char qrCodePayload[256];

// Task to read QR codes
void onQrCodeTask(void *pvParameters)
{
  struct QRCodeData qrCodeData;

  while (true)
  {
    if (reader.receiveQrCode(&qrCodeData, 100))
    {
      Serial.println("Found QRCode");
      if (qrCodeData.valid)
      {
        Serial.print("Payload: ");
        Serial.println((const char *)qrCodeData.payload);
        strcpy(qrCodePayload, (const char *)qrCodeData.payload);
      }
      else
      {
        Serial.print("Invalid: ");
        Serial.println((const char *)qrCodeData.payload);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  // Setup WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup QR code reader
  reader.setup();
  Serial.println("Setup QRCode Reader");
  reader.beginOnCore(1);
  Serial.println("Begin on Core 1");
  xTaskCreate(onQrCodeTask, "onQrCode", 4 * 1024, NULL, 4, NULL);
}

void readResponse(WiFiClient *client) {
  unsigned long timeout = millis();
  while (client->available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client->stop();
      return;
    }
  }

  while (client->available()) {
    String line = client->readStringUntil('\r');
    Serial.print(line);
  }

  Serial.printf("\nClosing connection\n\n");
}

void sendDataToThingSpeak(const char* payload) {
  WiFiClient client;
  String footer = String(" HTTP/1.1\r\n") + "Host: " + String(host) + "\r\n" + "Connection: close\r\n\r\n";

  if (!client.connect(host, httpPort)) {
    return;
  }

  String url = "/update?api_key=" + writeApiKey + "&field1=" + payload;
  client.print("GET " + url + footer);
  readResponse(&client);
}

void loop()
{
  sendDataToThingSpeak(qrCodePayload);
  delay(10000);
}
