#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* apSSID = "ESP32-Chat";
const char* apPass = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);
string Command = "";

const char* htmlPage =
"<!DOCTYPE html><html><head><title>ESP32 Chat</title></head><body>"
"<h2>ESP32 Chat</h2>"
"<div id='log' style='border:1px solid #ccc; height:200px; overflow:auto; padding:5px;'></div>"
"<input id='msg' type='text' placeholder='Type a message'/>"
"<button onclick='sendMsg()'>Send</button>"
"<script>"
"var ws = new WebSocket('ws://' + location.hostname + ':81/');"
"var log = document.getElementById('log');"
"ws.onopen = function(){ addLine('** connected **'); };"
"ws.onclose = function(){ addLine('** disconnected **'); };"
"ws.onmessage = function(e){ addLine('ESP32: ' + e.data); };"
"function sendMsg(){"
"  var i = document.getElementById('msg');"
"  ws.send(i.value);"
"  i.value = '';"
"}"
"function addLine(t){"
"  log.innerHTML += t + '<br>';"
"  log.scrollTop = log.scrollHeight;"
"}"
"</script></body></html>";

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    String reply = "";
    for (size_t i = 0; i < length; i++)
    {
      reply = reply + (char)payload[i];
    }
    Serial.printf("From browser: %s\n", reply);
    String broadCastMessage = "User: " +  String(num) + " " + reply;
    webSocket.broadcastTXT(broadCastMessage);
    Command = broadCastMessage
  } else if (type == WStype_CONNECTED) {
    Serial.printf("Client %u connected\n", num);
    webSocket.sendTXT(num, "hello from ESP32");
  }
}


// ============================================================================
//                        SETUP
// ============================================================================
void setup() 
{
    Serial.begin(115200);

    WiFi.softAP(apSSID, apPass);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());      // almost always 192.168.4.1

    server.on("/", []() { server.send(200, "text/html", htmlPage); });
    server.begin();
    webSocket.begin();
    webSocket.onEvent(onWsEvent);

}
    

// ============================================================================
//                        MAIN LOOP
// ============================================================================
void loop() 
{
  server.handleClient();
  webSocket.loop();
  static unsigned long last = 0;
  if (millis() - last > 5000) 
  {       // every 5 seconds
    last = millis();
    //webSocket.broadcastTXT("ping from ESP32 at " + String(millis()));
  }
}


