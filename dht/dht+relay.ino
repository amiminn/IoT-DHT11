
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Replace with your network credentials
const char* ssid = "wifiku";
const char* password = "12121212";

#define DHTPIN D1     // Digital pin connected to the DHT sensor
int RELAY = D2;

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
// #define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 10000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta http-equiv="X-UA-Compatible" content="IE=edge" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Fetch api</title>
    <style>
      body {
        text-align: center;
      }
    </style>
  </head>
  <body>
    <h2>Temperatur</h2>
    <h1 id="temp"></h1>

    <br />

    <h2>Kelembapan</h2>
    <h1 id="humd"></h1>

    <br />

    <button id="btnKirim">kirim</button>

    <br />

    <a id="db"></a>
    <p id="timer"></p>
    <script
      src="https://code.jquery.com/jquery-3.6.0.min.js"
      integrity="sha256-/xUj+3OJU5yExlq6GSYGSHk7tPXikynS7ogEvDej/m4="
      crossorigin="anonymous"
    ></script>
    <script>
      $(document).ready(() => {
        async function send() {
          const url = "https://db-nuklir.herokuapp.com/db-time";
          const database = "https://db-nuklir.herokuapp.com/database";

          const get = await $.get(url, (res) => {
            return res;
          });

          function dataPost(datenya, tempnya, humdnya) {
            $.post(
              database,
              {
                temp: tempnya,
                humd: humdnya,
                date: datenya,
              },
              (res) => {
                return res;
              }
            );
          }

          i = 1;
          setInterval(() => {
            dataPost(Date.now(), $("#temp").html(), $("#humd").html());
            $("#btnKirim").html(`running ... (${i})`);
            i++;
          }, get.time);
        }

        function fromNodeMcu() {
          $.get("http://192.168.43.177/temperature", (res) => {
            $("#temp").html(res);
          });
          $.get("http://192.168.43.177/humidity", (res) => {
            $("#humd").html(res);
          });
        }

        setInterval(() => {
          $("#timer").html(new Date());
          fromNodeMcu();
        }, 1000);

        $("#btnKirim").click(() => {
          $("#btnKirim").html(`running ... (0)`);
          $("#db")
            .html("view Database")
            .attr("href", "https://db-nuklir.herokuapp.com/database")
            .attr("target", "blank");
          send();
        });

        $("#btnRelayOn").click(() => {
          $.get("/onrelay", () => {
            $("#relay").html("RELAY STATUS: ON");
          });
        });

        $("#btnRelayOff").click(() => {
          $.get("/offrelay", () => {
            $("#relay").html("RELAY STATUS: OFF");
          });
        });

      });
    </script>
  </body>
</html>


)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });

pinMode(RELAY, OUTPUT);

 server.on("/onrelay", [](){
    server.send(200, "text/html", page);
    digitalWrite(RELAY, HIGH);
    delay(1000);
  });
  server.on("/offrelay", [](){
    server.send(200, "text/html", page);
    digitalWrite(RELAY, LOW);
    delay(1000); 
  });

  // Start server
  server.begin();
}
 
void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
    }
  }

  server.handleClient();
}