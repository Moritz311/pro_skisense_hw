#include <WiFiNINA.h>
#include "secrets.h" // WIFI_SSID / WIFI_PASS, siehe WIFI/secrets.example.h
WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 3000); // ohne USB (Akkubetrieb) nicht ewig warten

  if (WiFi.beginAP(WIFI_SSID, WIFI_PASS) != WL_AP_LISTENING) {
    Serial.println("Fehler beim Starten des AP!");
    while (true);
  }

  delay(5000);
  Serial.print("Access Point aktiv: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client verbunden");
    String req = client.readStringUntil('\r');
    Serial.print("Request: ");
    Serial.println(req);
    client.flush();

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println();
    client.println("Hallo vom RP2040!");
    client.stop();
  }
}
