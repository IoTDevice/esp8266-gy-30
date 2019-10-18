#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Wire.h>
#include <BH1750.h>

const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
const int httpPort = 80;
String deviceName = "光照传感器";
String version = "1.0";
ESP8266WebServer server(httpPort);

BH1750 lightMeter(0x23);

// web服务器的根目录
void handleRoot() {
  server.send(200, "text/html", "<h1>this is index page from esp8266!</h1>");
}
// 设备改名的API
void handleDeviceRename(){
  String message = "{\"code\":0,\"message\":\"success\"}";
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="name")
    {
      deviceName = server.arg(i);
    }
  }
  server.send(200, "application/json", message);
}

// 获取status
void handleCurrentStatus(){
  String message;
  float lux = lightMeter.readLightLevel();
  message = "{\"lightLevel\":"+String(lux)+
  ",\"code\":0,\"message\":\"success\"}";
  server.send(200, "application/json", message);
}

// 页面或者api没有找到
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Wire.begin(4, 5);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  WiFi.mode(WIFI_STA);
  // 选取一种连接路由器的方式 
  // WiFi.begin(ssid, password);
  WiFi.beginSmartConfig();

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }

  if (MDNS.begin("bh1750-"+String(ESP.getFlashChipId()))) {
    // Serial.println("MDNS responder started");
  }

  MDNS.addService("iotdevice", "tcp", httpPort);
  MDNS.addServiceTxt("iotdevice", "tcp", "name", deviceName);
  MDNS.addServiceTxt("iotdevice", "tcp", "model", "com.iotserv.devices.lightLevel");
  MDNS.addServiceTxt("iotdevice", "tcp", "mac", WiFi.macAddress());
  MDNS.addServiceTxt("iotdevice", "tcp", "id", ESP.getSketchMD5());
  MDNS.addServiceTxt("iotdevice", "tcp", "ui-support", "web,native");
  MDNS.addServiceTxt("iotdevice", "tcp", "ui-first", "native");
  MDNS.addServiceTxt("iotdevice", "tcp", "author", "Farry");
  MDNS.addServiceTxt("iotdevice", "tcp", "email", "newfarry@126.com");
  MDNS.addServiceTxt("iotdevice", "tcp", "home-page", "https://github.com/iotdevice");
  MDNS.addServiceTxt("iotdevice", "tcp", "firmware-respository", "https://github.com/iotdevice/esp8266-gy-30");
  MDNS.addServiceTxt("iotdevice", "tcp", "firmware-version", version);

  server.on("/", handleRoot);
  server.on("/rename", handleDeviceRename);
  server.on("/status", handleCurrentStatus);
  // about this device
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "{\"code\":1,\"message\":\"fail\"}" : "{\"code\":0,\"message\":\"success\"}");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      WiFiUDP::stopAll();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        
      } else {
        
      }
    }
    yield();
  });

  server.on("/ota", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  // Serial.println("HTTP server started");
  MDNS.addService("iotdevice", "tcp", httpPort);
}

void loop() {
  MDNS.update();
  server.handleClient();
}
