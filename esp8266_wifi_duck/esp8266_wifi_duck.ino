#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <EEPROM.h>
#include "data.h"
#include <ESP8266WiFiMulti.h>

#include "Settings.h"

ESP8266WiFiMulti WiFiMulti;

#define BAUD_RATE 57200
#define bufferSize 600
#define debug false

/* ============= CHANGE WIFI CREDENTIALS ============= */
const char *ssid = "Sparkle";
const char *password = "password"; //min 8 chars
const char *host = "ESP";
/* ============= ======================= ============= */

AsyncWebServer server(80);

FSInfo fs_info;
Settings settings;
bool shouldReboot = false;

//Web stuff
extern const uint8_t data_homeHTML[] PROGMEM;
extern const uint8_t data_updateHTML[] PROGMEM;
extern const uint8_t data_styleCSS[] PROGMEM;
extern const uint8_t data_functionsJS[] PROGMEM;
extern const uint8_t data_liveHTML[] PROGMEM;
extern const uint8_t data_infoHTML[] PROGMEM;
extern const uint8_t data_nomalizeCSS[] PROGMEM;
extern const uint8_t data_skeletonCSS[] PROGMEM;
extern const uint8_t data_settingsHTML[] PROGMEM;
extern const uint8_t data_viewHTML[] PROGMEM;

extern String formatBytes(size_t bytes);

//Script stuff
bool runLine = false;
bool runScript = false;
File script;

uint8_t scriptBuffer[bufferSize];
uint8_t scriptLineBuffer[bufferSize];
int bc = 0; //buffer counter
int lc = 0; //line buffer counter


String getContentType(String filename, AsyncWebServerRequest *request) {
  if (request->hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path, AsyncWebServerRequest *request) {
  digitalWrite(2, LOW);
  if (path.endsWith("/")) {
    path += "index.html";
  }

  String contentType = getContentType(path, request);

  if (SPIFFS.exists(path + ".gz")) {
    path += ".gz";
    AsyncWebServerResponse *response = request->beginResponse_P(200, contentType, "");
    response->addHeader("Content-Encoding", "gzip");
    request->send(SPIFFS, path, contentType, response);

    if (debug) Serial.println("HTTP 200: " + path);
    digitalWrite(2, HIGH);
    return true;
  } else if (SPIFFS.exists(path)) {
    request->send(SPIFFS, path, contentType);

    if (debug) Serial.println("HTTP 200: " + path);
    digitalWrite(2, HIGH);
    return true;
  }
  if (debug) Serial.println("HTTP 404: " + path);
  digitalWrite(2, HIGH);
  return false;
}

void handleFileDelete(AsyncWebServerRequest *request) {
  if (request->args() == 0) {
    AsyncWebServerResponse *response = request->beginResponse_P(500, "text/plain", "BAD ARGS");
    request->send(response);
    return;
  }
  String path = request->arg(0u);
  if (debug) Serial.println("handleFileDelete: " + path);
  if (path == "/") {
    AsyncWebServerResponse *response = request->beginResponse_P(500, "text/plain", "BAD PATH");
    request->send(response);
    return;
  }

  if (!SPIFFS.exists(path)) {
    AsyncWebServerResponse *response = request->beginResponse_P(404, "text/plain", "FileNotFound");
    request->send(response);
  }
  SPIFFS.remove(path);
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", "");
  request->send(response);
  path = String();
}

void handleFileCreate(AsyncWebServerRequest *request) {
  if (request->args() == 0) {
    AsyncWebServerResponse *response = request->beginResponse_P(500, "text/plain", "BAD ARGS");
    request->send(response);
    return;
  }

  String path = request->arg(0u);
  if (debug) Serial.println("handleFileCreate: " + path);
  if (path == "/") {
    AsyncWebServerResponse *response = request->beginResponse_P(500, "text/plain", "BAD PATH");
    request->send(response);
    return;
  }
  if (SPIFFS.exists(path)) {
    AsyncWebServerResponse *response = request->beginResponse_P(500, "text/plain", "FILE EXISTS");
    request->send(response);
    return;
  }
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else {
    AsyncWebServerResponse *response = request->beginResponse_P(500, "text/plain", "BAD ARGS");
    request->send(response);
    return;
  }
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", "");
  request->send(response);
  path = String();
}

void handleFileList(AsyncWebServerRequest *request) {
  if (!request->hasArg("dir")) {
    AsyncWebServerResponse *response = request->beginResponse_P(500, "text/plain", "BAD ARGS");
    request->send(response);
    return;
  }

  String path = request->arg("dir");
  if (debug) Serial.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  request->send(200, "text/json", output);
}


void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  File f;

  if (!filename.startsWith("/")) filename = "/" + filename;

  if (!index) f = SPIFFS.open(filename, "w"); //create or trunicate file
  else f = SPIFFS.open(filename, "a"); //append to file (for chunked upload)

  if (debug) Serial.write(data, len);
  f.write(data, len);

  if (final) { //upload finished
    if (debug) Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
    f.close();
  }
}

void send404(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(404, "text/html", "<h1>Oops!</h1><h2>404 NotFound</h2>");
  request->send(response);
}

void sendToIndex(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
  response->addHeader("Location", "/");
  request->send(response);
}

void sendSettings(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", data_settingsHTML, sizeof(data_settingsHTML));
  request->send(response);
}

void setup() {

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  Serial.begin(BAUD_RATE);

  if (debug) {
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();
    Serial.println("Debug:");
    Serial.printf("Flash real id:   %08X\r\n", ESP.getFlashChipId());
    Serial.printf("Flash real size: %u\r\n", realSize);
    Serial.printf("Flash ide  size: %u\r\n", ideSize);
    Serial.printf("Flash ide speed: %u\r\n", ESP.getFlashChipSpeed());
    Serial.printf("Flash ide  mode: %s\r\n\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
  }

  EEPROM.begin(4096);
  SPIFFS.begin();

  if (debug) {
    SPIFFS.info(fs_info);
    Serial.print("\r\n------ SPIFFS info ------\r\n");
    Serial.printf("Total   bytes:   %u\r\n", fs_info.totalBytes);
    Serial.printf("Used    bytes:   %u\r\n", fs_info.usedBytes);
    Serial.printf("Block   size:    %u\r\n", fs_info.blockSize);
    Serial.printf("Page    size:    %u\r\n", fs_info.pageSize);
    Serial.printf("Max open files:  %u\r\n", fs_info.maxOpenFiles);
    Serial.printf("Max path length: %u\r\n", fs_info.maxPathLength);
    Serial.print("------- File list -------\r\nName\t\tSize\r\n\n");
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      Serial.print(dir.fileName());
      Serial.print("\t");
      Serial.println(dir.fileSize());
    }
    Serial.print("-------------------------\r\n\n");
    Serial.println("\nstarting...\nSSID: " + (String)ssid + "\nPassword: " + (String)password);
  }
  digitalWrite(2, HIGH);

  settings.load();
  if (debug) settings.print();

  if (settings.autoExec) {
    String _name = (String)settings.autostart;
    script = SPIFFS.open("/" + _name, "r");
    runScript = true;
    runLine = true;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(settings.ssid, settings.password, settings.channel, settings.hidden);
  WiFi.hostname(host);

  // Connect AP here
  // WiFiMulti.addAP("name", "password");
  WiFiMulti.addAP("weslie", "zaj&1999");
  WiFiMulti.addAP("QwQ");
  WiFiMulti.run();

  if (debug) {
    Serial.print("Connect WiFi");
    while (WiFiMulti.run() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.print("\r\n");

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // ===== WebServer ==== //
  MDNS.addService("http", "tcp", 80);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!handleFileRead("/edit.html", request)) request->redirect("/home.html");
  });
  
  server.on("/home.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", data_homeHTML, sizeof(data_homeHTML));
    request->send(response);
  });

  server.on("/live.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", data_liveHTML, sizeof(data_liveHTML));
    request->send(response);
  });

  server.on("/view.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", data_viewHTML, sizeof(data_viewHTML));
    request->send(response);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", data_styleCSS, sizeof(data_styleCSS));
    request->send(response);
  });

  server.on("/normalize.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", data_nomalizeCSS, sizeof(data_nomalizeCSS));
    request->send(response);
  });

  server.on("/skeleton.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", data_skeletonCSS, sizeof(data_skeletonCSS));
    request->send(response);
  });

  server.on("/functions.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", data_functionsJS, sizeof(data_functionsJS));
    request->send(response);
  });

  server.on("/info.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", data_infoHTML, sizeof(data_infoHTML));
    request->send(response);
  });

  server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    sendSettings(request);
  });

  server.on("/settings.html", HTTP_POST, [](AsyncWebServerRequest * request) {

    if (request->hasArg("ssid")) {
      String _ssid = request->arg("ssid");
      settings.ssidLen = _ssid.length();
      _ssid.toCharArray(settings.ssid, 32);
      if (debug) Serial.println("new SSID = '" + _ssid + "'");
    }
    if (request->hasArg("pswd")) {
      String _pswd = request->arg("pswd");
      settings.passwordLen = _pswd.length();
      _pswd.toCharArray(settings.password, 32);
      if (debug) Serial.println("new password = '" + _pswd + "'");
    }
    if (request->hasArg("autostart")) {
      String _autostart = request->arg("autostart");
      settings.autostartLen = _autostart.length();
      _autostart.toCharArray(settings.autostart, 32);
      if (debug) Serial.println("new autostart = '" + _autostart + "'");
    }
    if (request->hasArg("ch")) settings.channel = request->arg("ch").toInt();
    if (request->hasArg("hidden")) settings.hidden = true;
    else settings.hidden = false;
    if (request->hasArg("autoExec")) settings.autoExec = true;
    else settings.autoExec = false;

    settings.save();
    if (debug) settings.print();

    sendSettings(request);
  });

  server.on("/settings.json", HTTP_GET, [](AsyncWebServerRequest * request) {
    String output = "{";
    output += "\"ssid\":\"" + (String)settings.ssid + "\",";
    output += "\"password\":\"" + (String)settings.password + "\",";
    output += "\"channel\":" + String((int)settings.channel) + ",";
    output += "\"hidden\":" + String((int)settings.hidden) + ",";
    output += "\"autoExec\":" + String((int)settings.autoExec) + ",";
    output += "\"autostart\":\"" + (String)settings.autostart + "\"";
    output += "}";
    request->send(200, "text/json", output);
  });

  server.on("/list.json", HTTP_GET, [](AsyncWebServerRequest * request) {
    SPIFFS.info(fs_info);
    Dir dir = SPIFFS.openDir("");
    String output;
    output += "{";
    output += "\"totalBytes\":" + (String)fs_info.totalBytes + ",";
    output += "\"usedBytes\":" + (String)fs_info.usedBytes + ",";
    output += "\"list\":[ ";
    while (dir.next()) {
      File entry = dir.openFile("r");
      String filename = String(entry.name()).substring(1);
      output += '{';
      output += "\"n\":\"" + filename + "\",";//name
      output += "\"s\":\"" + formatBytes(entry.size()) + "\"";//size
      output += "},";
      entry.close();
    }
    output = output.substring(0, output.length() - 1);
    output += "]}";
    request->send(200, "text/json", output);
  });

  server.on("/script", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasArg("name")) {
      String _name = request->arg("name");
      request->send(SPIFFS, "/" + _name, "text/plain");
    } else send404(request);
  });

  server.on("/run", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (request->hasArg("name")) {
      String _name = request->arg("name");
      script = SPIFFS.open("/" + _name, "r");
      runScript = true;
      runLine = true;
      request->send(200, "text/plain", "true");
    }
    else if (request->hasArg("script")) {
      Serial.println(request->arg("script"));
      request->send(200, "text/plain", "true");
    }
    else send404(request);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (request->hasArg("name") && request->hasArg("script")) {
      String _name = request->arg("name");
      String _script = request->arg("script");
      File f = SPIFFS.open("/" + _name, "w");
      if (f) {
        f.print(_script);
        request->send(200, "text/plain", "true");
      }
      else request->send(200, "text/plain", "false");
    }
    else send404(request);
  });


  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasArg("name")) {
      String _name = request->arg("name");
      SPIFFS.remove("/" + _name);
      request->send(200, "text/plain", "true");
    } else send404(request);
  });

  server.on("/rename", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasArg("name") && request->hasArg("newName")) {
      String _name = request->arg("name");
      String _newName = request->arg("newName");
      SPIFFS.rename("/" + _name, "/" + _newName);
      request->send(200, "text/plain", "true");
    }
    else send404(request);
  });

  server.on("/format", HTTP_GET, [](AsyncWebServerRequest * request) {
    SPIFFS.format();
    request->send(200, "text/plain", "true");
    sendToIndex(request);
  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    sendToIndex(request);
  }, handleUpload);

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
    response->addHeader("Location", "/info.html");
    request->send(response);
  });

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest * request) {
    shouldReboot = true;
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest * request) {
    settings.reset();
    request->send(200, "text/plain", "true");
    sendToIndex(request);
  });

  //update
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      if (debug) Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
        if (debug) Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        if (debug) Serial.printf("Update Success: %uB\n", index + len);
      } else {
        if (debug) Update.printError(Serial);
      }
    }
  });

  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!handleFileRead("/edit.html", request)) send404(request);
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", "");
    request->send(response);
  }, handleUpload);

  server.onNotFound([](AsyncWebServerRequest * request) {
    if (!handleFileRead(request->url(), request)) {
      send404(request);
    }
  });

  server.begin();

  if (debug) Serial.println("started");
}

void sendBuffer() {
  for (int i = 0; i < bc; i++) Serial.write((char)scriptBuffer[i]);
  runLine = false;
  bc = 0;
}

void addToBuffer() {
  if (bc + lc > bufferSize) sendBuffer();
  for (int i = 0; i < lc; i++) {
    scriptBuffer[bc] = scriptLineBuffer[i];
    bc++;
  }
  lc = 0;
}

void loop() {
  if (shouldReboot) ESP.restart();

  if (Serial.available()) {
    uint8_t answer = Serial.read();
    if (answer == 0x99) {
      if (debug) Serial.println("done");
      runLine = true;
    }
    else {
      String command = (char)answer + Serial.readStringUntil('\n');
      command.replace("\r", "");
      if (command == "reset") {
        settings.reset();
        shouldReboot = true;
      }
    }
  }

  if (runScript && runLine) {
    if (script.available()) {
      uint8_t nextChar = script.read();
      if (debug) Serial.write(nextChar);
      scriptLineBuffer[lc] = nextChar;
      lc++;
      if (nextChar == 0x0D || lc == bufferSize) addToBuffer();
    } else {
      addToBuffer();
      if (bc > 0) sendBuffer();
      runScript = false;
      script.close();
    }
  }

}
