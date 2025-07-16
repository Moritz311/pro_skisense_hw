#include <WiFiNINA.h>

//server section
WiFiServer server(80);
const String ssid = "SkiSensor";
const String pwd = "12345678";

//analog Pins
const int fsrPinS1 = A0;
const int fsrPinS2 = A1;
const int fsrPins[2] = {fsrPinS1,fsrPinS2};

//overgiven data struct
struct Measurement{
  unsigned long timestamp;
  uint16_t values[2]; //variabl with FSR420 sensors
};

//timetracking
unsigned long startTime = 0;
bool measuring = false;
bool dataSent = false;

const unsigned long MEASURE_INTERVAL = 100; // ms
const unsigned long TOTAL_DURATION = 1 * 60 * 1000; // 10 min - now 1 minute
const unsigned long CHECK_START = 0.8 * 60 * 1000; // start sending window

const int MAX_SAMPLES = 6000;
Measurement data[MAX_SAMPLES];
int count = 0;
unsigned long lastSample = 0;


void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (WiFi.beginAP(ssid.c_str(), pwd.c_str()) != WL_AP_LISTENING) {
    Serial.println("AP-Start fehlgeschlagen!");
    while (true);
  }

  delay(5000); // time to start

  Serial.print("Access Point IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client && client.connected()) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /start") >= 0 && !measuring) {
      Serial.println("Messung gestartet");
      measuring = true;
      startTime = millis();
      count = 0;
      dataSent = false;
    } else if(request.indexOf("GET /data")>=0 && measuring){
        Serial.println("Daten abgefragt");
        sendData(client);
        startTime = millis();
    } else if (request.indexOf("GET /stop") >= 0 && measuring) {
      Serial.println("Manuell gestoppt → Messung beendet");
      sendData(client);
      measuring = false;
    }

    // JSON-Antwort senden
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println("OK");

    client.stop();
  }

  if (measuring) {
    unsigned long now = millis();

    // Sensorwert speichern
    if (now - lastSample >= MEASURE_INTERVAL && count < MAX_SAMPLES) {
      lastSample = now;
      data[count].timestamp = now - startTime;
      data[count].values[0] = analogRead(fsrPins[0]);
      data[count].values[1] = analogRead(fsrPins[1]);
      count++;
    }

    // Automatisches Senden bei Verbindung nach 8 Minuten
    if ((now - startTime >= CHECK_START) && !dataSent) {
      WiFiClient sendClient = server.available(); // Versuch neuen Client zu erkennen
      if (sendClient && sendClient.connected()) {
        Serial.println("Client erkannt – Daten werden gesendet");
        sendData(client);
        dataSent = true;
      } else if (now - startTime >= TOTAL_DURATION) {
        Serial.println("Zeit abgelaufen – keine Verbindung erkannt");
        measuring = false;
      }
    }
  }
}

void sendData(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println();

  client.print("{\"data\":[");
  for (int i = 0; i < count; i++) {
    client.print("{\"t\":");
    client.print(data[i].timestamp);
    client.print(",\"v\":[");
    client.print(data[i].values[0]);
    client.print(",");
    client.print(data[i].values[1]);
    client.print("]}");
    if (i < count - 1) client.print(",");
  }
  client.println("]}");

  count = 0;
  client.stop();
}
