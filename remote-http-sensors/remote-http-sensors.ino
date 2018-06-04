/**
 * Remote HTTP sensors for Thermorasp (just temperature for now)
 * @author Daniele Ricci
 */

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <mDNSResolver.h>

// WiFi configuration
#define WIFI_SSID      "ssid"
#define WIFI_PASSWORD  "password"

// HTTP listen port
#define LISTEN_PORT   9000

// Thermostat registration
#define THERMOSTAT_HOST       "thermostat.local"
#define THERMOSTAT_PORT       80
#define THERMOSTAT_API_REG    "/api/sensors/register"
#define THERMOSTAT_API_SEND   "/api/sensors/reading"
#define SENSOR_ID             "temp_bedroom"
#define SENSOR_PASSIVE        false
#define SEND_INTERVAL         60e6

// Sensor PIN
#define ONE_WIRE_BUS      D7  // GPIO13

#define LED_PWR           D3
#define LED_WIFI          D2
#define LED_REG           D1

WiFiServer server(LISTEN_PORT);
WiFiUDP udp;
mDNSResolver::Resolver resolver(udp);

// prepare a 1-Wire bus connection
OneWire oneWire(ONE_WIRE_BUS); 
// prepare a Dallas temperature connection
DallasTemperature sensors(&oneWire);

// for getting VCC
ADC_MODE(ADC_VCC);

void setup() {
  // turn on power LED
  lightOn(LED_PWR);

  // wait for serial to initialize
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }

  Serial.println();
  Serial.println();
  Serial.println("Initializing sensors");

  digitalWrite(ONE_WIRE_BUS, HIGH);
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP); // Mandatory to make it work on ESP8266

  // init sensors
  sensors.begin(); 

  // Connect to WiFi network
  connect();

  // Look for thermostat
  Serial.println("Searching for thermostat...");
  IPAddress ip = resolver.search(THERMOSTAT_HOST);
    if(ip != INADDR_NONE) {
      Serial.print("Found thermostat: ");
      Serial.println(ip);
    } else {
      Serial.println("Thermostat not found!");
  }

#if SENSOR_PASSIVE
  String localhost = String(SENSOR_ID) + ".local";

  // Register to the thermostat
  Serial.print("Registering to thermostat at ");
  Serial.print(ip);
  Serial.print(":");
  Serial.print(THERMOSTAT_PORT);
  Serial.print("...");
  if (registerSelf(ip, THERMOSTAT_PORT, localhost + ":" + LISTEN_PORT)) {
    Serial.println(" registered!");
    lightOn(LED_REG);
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
  Serial.print(localhost);
  Serial.print(":");
  Serial.print(LISTEN_PORT);
  Serial.println("/");
#else
  float temp = readTemperature();
  Serial.print("Temperature: ");
  Serial.println(temp);
  if (temp == -127) {
    Serial.println("Invalid reading, aborting.");
  }
  else {
    Serial.println("Sending reading to thermostat");

    HTTPClient http;
    http.begin("http://" + ip.toString() + ":" + THERMOSTAT_PORT + THERMOSTAT_API_SEND);
    http.addHeader("Content-Type", "application/json");

    String message = String("{") +
      "\"sensor_id\": \""+SENSOR_ID+"\","
      "\"type\": \"temperature\"," +
      "\"unit\": \"celsius\"," +
      "\"value\": \""+temp+"\"}";

    int httpCode = http.POST(message);
    Serial.print("Thermostat replied with ");
    Serial.println(httpCode);
    http.end();
  }

  ESP.deepSleep(SEND_INTERVAL);
#endif
}

void loop() {
#if SENSOR_PASSIVE
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
  else if (request.indexOf("/status") != -1)  {
    responseStatus(client);
  }
  else {
    responseError(client);
  }

  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
#endif
}

void connect() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  lightOn(LED_WIFI);

  resolver.setLocalIP(WiFi.localIP());

#if SENSOR_PASSIVE
  // Sensor will reply at SENSOR_ID.local
  if (!MDNS.begin(SENSOR_ID)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
#endif
}

void responseError(WiFiClient client) {
  client.println("HTTP/1.1 400 Bad Request");
  client.println("Content-Type: text/plain");
  client.println("");
  client.println("Bad request");
}

void responseStatus(WiFiClient client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("");
    client.print("{\"power\":");
    client.print(readPower());
    client.print("}");
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

float readPower() {
  return (float) ESP.getVcc() / 1024;
}

bool registerSelf(IPAddress address, int port, String localhost) {
  WiFiClient client;
  if (!client.connect(address, port)) {
    Serial.println("connection to thermostat failed");
    return false;
  }

  String sensor_mode = String(SENSOR_PASSIVE ? "passive" : "active");
  String message = String("{") +
     "\"id\": \""+SENSOR_ID+"\"," +
     "\"type\": \"temperature\"," +
     "\"data_mode\": \""+sensor_mode+"\"," +
     "\"protocol\": \"inet\"," +
     "\"address\": \"HTTP:http://" + localhost + "/temperature\"}";

  client.println(String("POST ") + THERMOSTAT_API_REG + " HTTP/1.1");
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

void lightOn(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
}

void lightOff(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

