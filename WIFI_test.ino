#include <WiFiNINA.h>
WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (WiFi.beginAP("SkiSensor", "12345678") != WL_AP_LISTENING) {
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
