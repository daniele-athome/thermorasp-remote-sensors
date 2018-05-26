/**
 * Remote HTTP sensors for Thermorasp (just temperature for now)
 * @author Daniele Ricci
 */

#include <ESP8266WiFi.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

// WiFi configuration
#define WIFI_SSID      "ssid"
#define WIFI_PASSWORD  "password"

// HTTP listen port
#define LISTEN_PORT   9000

// Thermostat registration
#define THERMOSTAT_HOST   "192.168.0.250"
#define THERMOSTAT_PORT   80
#define THERMOSTAT_API    "/api/sensors/register"
#define SENSOR_ID         "temp_bedroom"

// Sensor PIN
#define ONE_WIRE_BUS      D7  // GPIO13

WiFiServer server(LISTEN_PORT);

// prepare a 1-Wire bus connection
OneWire oneWire(ONE_WIRE_BUS); 
// prepare a Dallas temperature connection
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.println("Initializing sensors");

  digitalWrite(ONE_WIRE_BUS, HIGH);
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP); // Mandatory to make it work on ESP8266

  // init sensors
  sensors.begin(); 

  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Register to the thermostat
  Serial.print("Registering to thermostat at ");
  Serial.print(THERMOSTAT_HOST);
  Serial.print(":");
  Serial.print(THERMOSTAT_PORT);
  Serial.print("...");
  if (registerSelf(WiFi.localIP().toString() + ":" + LISTEN_PORT)) {
    Serial.println(" registered!");
  }
  else {
    Serial.println(" failed!");
  }

  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.print(LISTEN_PORT);
  Serial.println("/");
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
 
  // Handle requests 
  if (request.indexOf("/temperature") != -1)  {
    responseTemperature(client);
  }
  else {
    responseError(client);
  }

  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
}

void responseError(WiFiClient client) {
  client.println("HTTP/1.1 400 Bad Request");
  client.println("Content-Type: text/plain");
  client.println("");
  client.println("Bad request");
}

void responseTemperature(WiFiClient client) {
  float temp = readTemperature();
  Serial.print("Temperature: ");
  Serial.println(temp);
  if (temp == -127) {
  client.println("HTTP/1.1 500 Internal Server Error");
  client.println("Content-Type: text/plain");
  client.println("");
  client.println("Unable to read temperature");
  }
  else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("");
    client.print("{\"value\":");
    client.print(temp);
    client.print(",\"unit\":\"celsius\"}");
  }
}

float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

bool registerSelf(String localhost) {
  WiFiClient client;
  if (!client.connect(THERMOSTAT_HOST, THERMOSTAT_PORT)) {
    Serial.println("connection to thermostat failed");
    return false;
  }

  String message = String("{") +
     "\"id\": \"temp_bedroom\"," +
     "\"type\": \"temperature\"," +
     "\"data_mode\": \"passive\"," +
     "\"protocol\": \"inet\"," +
     "\"address\": \"HTTP:http://" + localhost + "/temperature\"}";

  client.println(String("POST ") + THERMOSTAT_API + " HTTP/1.1");
  client.println(String("Host: ") + THERMOSTAT_HOST);
  client.println("Content-type: application/json");
  client.println("Connection: close");
  client.println(String("Content-Length: ") + message.length());
  client.println("");
  client.println(message);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (line.indexOf("HTTP/1.1 201") != -1) {
      return true;
    }
    Serial.print(line);
  }

  return false;
}

