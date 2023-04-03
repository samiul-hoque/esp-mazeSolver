// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <TB6612FNG.h>
// Replace with your network credentials
const char* ssid = "Vinci's Workshop";
const char* password = "asdfzxcv";

bool ledState = 0;
#define ledPin 2

#define stby 25
//pin definitions: motor Left
#define motorL1 27
#define motorL2 14
#define motorLPwm 13

#define motorR1 15
#define motorR2 33
#define motorRPwm 26

//pin definitions: sensor
#define xShutL 17
#define xShutF 16
#define xShutR 32

//init motors object

Tb6612fng motors(25, 27, 14, 13, 15, 33, 26);

unsigned long currentMillis = 0;
unsigned long sensor1 = 0;
unsigned long sensor2 = 0;
unsigned long sensor3 = 0;


String globalSensorData = "";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>ESP Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <style>
      html {
        font-family: Arial, Helvetica, sans-serif;
        text-align: center;
      }
      h1 {
        font-size: 1.8rem;
        color: white;
      }
      h2 {
        font-size: 1.5rem;
        font-weight: bold;
        color: #143642;
      }
      .topnav {
        overflow: hidden;
        background-color: #143642;
      }
      body {
        margin: 0;
      }
      .content {
        padding: 30px;
        max-width: 600px;
        margin: 0 auto;
      }
      .card {
        background-color: #f8f7f9;
        box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, 0.5);
        padding-top: 10px;
        padding-bottom: 20px;
      }
      .button {
        padding: 15px 50px;
        font-size: 24px;
        text-align: center;
        outline: none;
        color: #fff;
        background-color: #0f8b8d;
        border: none;
        border-radius: 5px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
      }
      /*.button:hover {background-color: #0f8b8d}*/
      .button:active {
        background-color: #0f8b8d;
        box-shadow: 2 2px #cdcdcd;
        transform: translateY(2px);
      }
      .state {
        font-size: 1.5rem;
        color: #8c8c8c;
        font-weight: bold;
      }
      .column {
        float: left;
        width: 33.33%;
      }

      /* Clear floats after the columns */
      .row:after {
        content: "";
        display: table;
        clear: both;
      }
    </style>
    <title>ESP Maze Solver</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
  </head>
  <body>
    <div class="topnav">
      <h1>ESP Maze Solver</h1>
    </div>
    <div class="content">
      <div class="card">
        <h2>Sensor Output</h2>
        <p class="state"><span id="state">%STATE%</span></p>
        <div class="row">
          <div class="column">
            <p><button id="left" class="button">Left</button></p>
          </div>
          <div class="column">
            <p><button id="forward" class="button">Forward</button></p>
            <div class="column">
              <p><button id="reverse" class="button">Reverse</button></p>
            </div>
          </div>


          <div class="column">
            <p><button id="right" class="button">Right</button></p>
          </div>
        </div>
        
      </div>
    </div>
    <script>
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;
      window.addEventListener("load", onLoad);
      function initWebSocket() {
        console.log("Trying to open a WebSocket connection...");
        websocket = new WebSocket(gateway);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onmessage = onMessage; // <-- add this line
      }
      function onOpen(event) {
        console.log("Connection opened");
      }
      function onClose(event) {
        console.log("Connection closed");
        setTimeout(initWebSocket, 2000);
      }
      function onMessage(event) {
        document.getElementById("state").innerHTML = JSON.stringify(event.data);
      }
      function onLoad(event) {
        initWebSocket();
        initAutoFetch();
        initButton();
      }

      function forward() {
        websocket.send("forward");
      }
      function reverse() {
        websocket.send("reverse");
      }
      function left() {
        websocket.send("left");
      }
      function right() {
        websocket.send("right");
      }

      function initButton() {
        document.getElementById("forward").addEventListener("click", forward);
        document.getElementById("reverse").addEventListener("click", reverse);
        document.getElementById("left").addEventListener("click", left);
        document.getElementById("right").addEventListener("click", right);
      }

      function initAutoFetch() {
        setInterval(function () {
          websocket.send("toggle");
        }, 500);
      }
    </script>
  </body>
</html>
)rawliteral";

void notifyClients() {
  ws.textAll(String(ledState));
}

void sendTextToWs(){
  ws.textAll(globalSensorData);
}

void forward()
{
  Serial.println("forward");
  motors.drive(0.5,500);
}
void reverse()
{
  Serial.println("reverse");
  motors.drive(-0.5,500);
}
void left()
{
  Serial.println("left");
  motors.drive(-1.0, 1.0, 500);
}
void right()
{
  Serial.println("right");
  motors.drive(1.0, -1.0, 500);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "forward") == 0) {
      forward();
    }
     else if (strcmp((char*)data, "reverse") == 0) {
      reverse();
    }
     else if (strcmp((char*)data, "left") == 0) {
      left();
    }
     else if (strcmp((char*)data, "right") == 0) {
      right();
    }
    
    sendTextToWs();
    // }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  //motor init
  motors.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);
  // Start server
  server.begin();
}

String sensorParser(){
  //returns aggregated sensor data as string for webUI
  String data = "";
  currentMillis = millis();
  sensor1 = currentMillis;
  sensor2 = currentMillis+1;
  sensor2 = currentMillis+2; 

  data = "{Sensor1:" + String(sensor1) + ", Sensor2: " + String(sensor2) +", Sensor3: "+ String(sensor1) +"}" ;
  return data;
}




void loop() {
  ws.cleanupClients();
  globalSensorData = sensorParser();
}