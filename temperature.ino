#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Timer.h>

byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xCD };
IPAddress ip(192, 168, 100, 177);

EthernetServer server(80);

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

Timer temperatureTimer;

float currentTemperature = 0.0;

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  delay(1000);

  startEthernet();
  Serial.println("Started Ethernet Thermometer");

  // Start up the 1 Wire temperature sensor
  sensors.begin();
  saveTemperature();
  temperatureTimer.every(30*1000, saveTemperature);

  while(1) {
    handleRequests(); 
    temperatureTimer.update();
  }
}

void startEthernet()
{
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());  
}


void processGetRequest(String line)
{ 
  Serial.println("Processing get request");
}

void handleRequests() 
{
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    while (client.connected()) {
      if (client.available()) {
        String line = readHttpLine(client);

        if (line.indexOf("GET /") >= 0) {
          processGetRequest(line);  
        }

        if (line.equals("")) {
          sendHttpResponse(client);
          break;
        }
      }
    }

    delay(1);
    client.stop();
    Serial.println("client disonnected");
  }
}

String readHttpLine(EthernetClient client)
{
  String line = client.readStringUntil('\n');     
  line.replace("\r", "");

  return line;
}

void sendHttpResponse(EthernetClient client)
{
  Serial.println("Sending response now");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connnection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<body style=\"font-size:15vmax;text-align:center;font-family:sans-serif;color:#1F2950;background:#388CBB;position: absolute;top:50%;left:50%;margin-right: -50%;transform: translate(-50%, -50%);\">");
  client.print(currentTemperature);
  client.print(" &deg;C");
  client.println("<body>");
  client.println("</html>");
}  

float getTemperature()
{ 
  float temperature = 0.0;
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  return sensors.getTempCByIndex(0);
}

void saveTemperature() {
  currentTemperature = getTemperature();
  Serial.print("Saved temperature: ");
  Serial.println(currentTemperature);
}

void loop()
{
}

