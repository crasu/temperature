#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Time.h> 
#include <Ethernet.h>
#include <EthernetUdp.h>"
#include <Timer.h>
#include <SD.h>
#include "Ntp.h"

#define CHIP_SELECT 4
#define SD_CS_PIN SS
#define LOG_TIME_DELTA 300

byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xCD };
IPAddress ip(192, 168, 100, 177);
EthernetServer server(80);

const int ONE_WIRE_PIN=2;
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

Timer temperatureTimer;

float currentTemperature1 = 0.0,
      currentTemperature2 = 0.0;

void setup(void)
{
  Serial.begin(9600);
  delay(1000);
  
  if (!SD.begin(CHIP_SELECT)) {
    Serial.println(F("SDCard failed"));
    return;
  }
  
  startEthernet();
  initNtp();
  
  sensors.begin();
  saveTemperature();
  temperatureTimer.every(30*1000, saveTemperature);

  Serial.println(F("Started!"));
}

void startEthernet()
{
  Ethernet.begin(mac, ip);
  server.begin();
}

void handleRequests() 
{
  EthernetClient client = server.available();
  if (client) {
    Serial.println(F("new client"));
    while (client.connected()) {
      if (client.available()) {
        String line = readHttpLine(client);

        if (line.startsWith("GET /data.json")) {
          sendFileResponse(client);
          break;
        }

        if (line.equals("")) {
          sendHtmlResponse(client);
          break;
        }
      }
    }

    delay(1);
    client.stop();
  }
}

String readHttpLine(EthernetClient client)
{
  String line = client.readStringUntil('\n');     
  line.replace("\r", "");

  return line;
}

void sendFileResponse(EthernetClient client)
{
  Serial.println("Sending log.json");
  sendHeader(client, "text/plain");

  File dataFile = SD.open("log.txt");

  while (dataFile.available()) {
    client.write(dataFile.read());
  }
 
  dataFile.close();
}

void sendHeader(EthernetClient client, char* contentType)
{
  Serial.println(F("HTTP response"));
  client.print(F("HTTP/1.1 200 OK\nContent-Type: "));
  client.println(contentType);
  client.println(F("Connnection: close\n"
    "\n"));
}

void sendHtmlResponse(EthernetClient client)
{
  sendHeader(client, "text/html");

  client.println(F("<!DOCTYPE HTML>"
    "<html>"
    "<body style="
    "\"font-size:12vmin;"
    "text-align:center;"
    "font-family:sans-serif;"
    "color:#1F2950;"
    "background:#388CBB;"
    "position: absolute;"
    "top:50%;"
    "left:50%;"
    "margin-right: -50%;"
    "transform: translate(-50%, -50%);\">"));
  client.print("Out: ");
  client.print(currentTemperature1);
  client.print(" &deg;C<br> In: ");
  client.print(currentTemperature2);
  client.print(" &deg;C");
  client.println(F("<body>"
    "</html>"));
}

void saveTemperature() {
  static time_t oldTime = 0;
  
  Serial.println(F("Getting temps"));
  sensors.requestTemperatures();
  currentTemperature1 = sensors.getTempCByIndex(0);
  currentTemperature2 = sensors.getTempCByIndex(1);

  Serial.println(currentTemperature1);
  Serial.println(currentTemperature2);

  if (timeStatus() != timeSet || now() - oldTime < LOG_TIME_DELTA) {
    return;
  }
  
  File dataFile = SD.open("log.txt", FILE_WRITE);

  if (dataFile) {
    oldTime = now();
    dataFile.print(now());
    dataFile.print(",");
    dataFile.print(currentTemperature1);
    dataFile.print(",");
    dataFile.println(currentTemperature2);
    dataFile.close();
  }
  
  Serial.println(F("Saved temps"));
}

void loop()
{
  handleRequests(); 
  temperatureTimer.update();
}

