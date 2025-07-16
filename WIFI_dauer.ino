#include <WiFiNINA.h>

WiFiServer server(80);
const char* ssid = "SkiSensor";
const char* pwd  = "12345678";

const int fsr1 = A0;
const int fsr2 = A1;

unsigned long lastSample = 0;
const unsigned long INTERVAL = 100;

unsigned long lastping = 0;
const unsigned long CLIENT_TIMEOUT = 8000;
bool streamActive = false;
unsigned long now = 0;

void html(WiFiClient client){

        client.println(R"rawliteral(
  <!DOCTYPE html>

  <html>
  <head>
    <meta charset="utf-8">
    <title>Live Sensor</title>
    <style>
      body { font-family: sans-serif; padding: 1em; }
      .value { font-size: 1.5em; margin-bottom: 0.5em; }
    </style>
  </head>
  <body>
    <h2>Live FSR-Daten</h2>
    <div class="value" id="fsr">Warte auf Daten...</div>

    <script>
      const fsr = document.getElementById("fsr");
      if (window.source && window.source.close) {
        window.source.close();
      }
      window.source = new EventSource("/live");
      window.source.onmessage = function(e) {
        const d = JSON.parse(e.data);
        fsr.textContent = `Zeit: ${d.t} ms → Sensor1: ${d.v[0]}, Sensor2: ${d.v[1]}`;
      };
      setInterval(() => {
        source.dispatchEvent(new MessageEvent("ping", { data: "keepalive" }));
      }, 2000);

    </script>
  </body>
  </html>
  )rawliteral");
}

void handleClient(WiFiClient client, String req){
  if (req.indexOf("GET /live") >= 0) {
      if (streamActive) {
        client.println("HTTP/1.1 429 Too Many Requests");
        client.println("Connection: close");
        client.println();
        client.stop();
        return;
      }
      streamActive = true;

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/event-stream");
      client.println("Cache-Control: no-cache");
      client.println("Connection: keep-alive");
      client.println();

      bool streamStarted = false;
      while (client.connected()) {
        now = millis();
        if (now - lastSample >= INTERVAL) {
          lastSample = now;
          int v1 = analogRead(fsr1);
          int v2 = analogRead(fsr2);

          client.print("data: {\"t\":");
          client.print(now);
          client.print(",\"v\":[");
          client.print(v1);
          client.print(",");
          client.print(v2);
          client.println("]}");
          client.println();


          if (!streamStarted) {
            streamActive = true;
            streamStarted = true;
            lastping = millis();
          }
          delay(10);
        }
        if (now >= lastping && now - lastping > CLIENT_TIMEOUT){
          unsigned long dif = now-lastping;
          Serial.println("Client hat keinen Ping mehr gesendet – Trenne."+String(dif)+","+String(now)+","+String(lastping));
          break;
        }
      }
      streamActive = false;
      client.stop();
    }
     else { // Serve UI
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();

      html(client);

      client.stop();
    } 
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  WiFi.beginAP(ssid, pwd);
  delay(3000);
  Serial.println("AP IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
  lastping=millis();
}

void loop() {
  WiFiClient incoming = server.available();
  if (incoming && incoming.connected()) {
    String req = incoming.readStringUntil('\r');
    incoming.flush();

    if (req.indexOf("GET /ping") >= 0) {
      lastping = millis();
      Serial.println("Ping erhalten " + String(now)+","+String(lastping));
      incoming.println("HTTP/1.1 200 OK");
      incoming.println("Content-Type: text/plain");
      incoming.println("Connection: close");
      incoming.println();
      incoming.println("pong");
      incoming.stop();
      return;
    }

    handleClient(incoming, req);
  }
}

