/*
// MAC Address

#include <Arduino.h>
// Complete Instructions to Get and Change ESP MAC Address: https://RandomNerdTutorials.com/get-change-esp32-esp8266-mac-address-arduino/
#include "WiFi.h"

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());
}

void loop()
{
}
*/

// ESP32 Receiver Sketch (ESP-NOW)
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
/*
  esp_now_init() : ESP-NOW 초기화 ESP-NOW 초기화하기 전에 Wi-Fi 초기화해야 함
  esp_now_add_peer() : 장치를 페어링하고 피어 MAC 주소를 인수로 전달
  esp_now_send() : ESP-NOW 통해 데이터 송신
  esp_now_register_send_cb() : 데이터 [전송 시] 실행되는 콜백 함수 등록. 메시지가 전송되면 함수가 호출됨. 전달 성공했는지 여부를 반환.
  esp_now_register_rcv_cb() : 데이터 [수신 시] 실행되는 콜백 함수를 등록. ESP-NOW를 통해 데이터가 수신되면 함수가 호출됨.
*/
/*
  ESP32 수신기 보드는 송신기 보드로부터 패킷을 수신하고
  웹 서버를 호스팅하여 최근 수신된 판독값을 표시
*/
#include <esp_now.h>
#include <WiFi.h>

#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>

// Replace with your network credentials (STATION)
const char *ssid = "dinfo";
const char *password = "daon7521";

// 데이터 수신할 구조체
// 송신자 스케치에 정의된 것과 동일해야 함
typedef struct struct_message
{
  // char a[32];
  // int b;
  // float c;
  // bool d;
  int id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

// Create a struct_message called incomingReadings
struct_message incomingReadings;

JSONVar board;

AsyncWebServer server(80);
// 새로운 판독값이 도착하면 웹 서버에 자동으로 표시하기 위해 SSE(Server-Sent Events) 사용
AsyncEventSource events("/events"); // 새 이벤트 소스 생성

// callback function that will be executed when data is received
// 콜백함수로 사용할 함수 정의
// 콜백함수는 데이터 수신 시 실행됨
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  // 송신자 MAC 주소 출력 Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  // snprintf() : 패킷통신을 하거나 버퍼에 원하는 문자열을 삽입, append 할때 자주 사용
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);

  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  // 수신된 정보로 json 문자열 변수 생성
  board["id"] = incomingReadings.id;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.hum;
  board["readingId"] = String(incomingReadings.readingId);
  String jsonString = JSON.stringify(board);

  // 수신된 모든 데이터를 수집한 후 jsonString 변수를 사용해
  // 해당 정보를 이벤트로 브라우저에 보냄
  events.send(jsonString.c_str(), "new_readings", millis());

  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf("Temp: %4.2f \n", incomingReadings.temp);
  Serial.printf("Hum: %4.2f \n", incomingReadings.hum);
  Serial.printf("readingID: %d \n", incomingReadings.readingId);
  Serial.println();
}

// web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #1 - TEMPERATURE</h4><p><span class="reading"><span id="t1"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt1"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #1 - HUMIDITY</h4><p><span class="reading"><span id="h1"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh1"></span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #2 - TEMPERATURE</h4><p><span class="reading"><span id="t2"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt2"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #2 - HUMIDITY</h4><p><span class="reading"><span id="h2"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh2"></span></p>
      </div>
    </div>
  </div>
<script>
// EventSource() 객체를 업데이트하고 업데이트를 보내는 페이지의 url 지정
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 // 이벤트 소스를 인스턴스화 한 후에는 다음을 사용하여 서버에서 메시지 수신을 시작할 수 있음
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2); // 소수부분 자릿수를 인수만큼 고정 후 그 값을 문자열로 반환
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
  document.getElementById("rh"+obj.id).innerHTML = obj.readingId;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup()
{
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  // WiFi.mode(WIFI_STA);
  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);

  // wifi / web 활성화/비활성화
  // /*
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  // */
  // wifi/web 활성화/비활성화 영역 끝

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  // esp_now_register_rcv_cb() : 데이터 [수신 시] 실행되는 콜백 함수를 등록.
  // ESP-NOW를 통해 데이터(ESP-NOW패킷)가 수신되면 함수가 호출됨.
  esp_now_register_recv_cb(OnDataRecv);

  // Handle Requests: 루트에서 esp32 ip주소에 액세스하는 경우 /url에 저장된 텍스트 보냄
  // index_html 웹 페이지를 구축하기 위한 변수
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html); });

  // 서버에서 이벤트 소스 설정
  events.onConnect([](AsyncEventSourceClient *client)
                   {
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000); });
  server.addHandler(&events);
  // 서버 시작
  server.begin();
}

// 5초마다 ping을 보내는 루프 : 서버가 아직 실행중인지 클라이언트 측에서 확인하는데 사용
void loop()
{
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS)
  {
    events.send("ping", NULL, millis());
    lastEventTime = millis();
  }
}