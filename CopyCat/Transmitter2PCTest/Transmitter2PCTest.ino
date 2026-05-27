#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* apSSID = "ESP32-ChatRoom";
const char* apPass = "HeiHeiHei";

WebServer server(80);
WebSocketsServer webSocket(81);
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
"  addLine('Me: ' + i.value);"
"  i.value = '';"
"}"
"function addLine(t){"
"  log.innerHTML += t + '<br>';"
"  log.scrollTop = log.scrollHeight;"
"}"
"</script></body></html>";

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    Serial.printf("From browser: %s\n", payload);
    String reply = "got '" + String((char*)payload) + "'";
    webSocket.sendTXT(num, reply);
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
    WiFi.softAP(apSSID,apPass);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
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
}