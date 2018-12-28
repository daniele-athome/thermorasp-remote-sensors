/**
 * Remote MQTT sensors for Thermorasp (just temperature for now)
 * @author Daniele Ricci
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <mDNSResolver.h>

// WiFi configuration
#define WIFI_SSID      "ssid"
#define WIFI_PASSWORD  "password"

// Thermostat configuration
#define THERMOSTAT_HOST       "thermostat.local"

// Sensor configuration
#define SENSOR_ID             "temp_living"
#define SENSOR_TOPIC          "homeassistant/thermorasp/sensor/" SENSOR_ID "/control"
// Data send interval
#define SEND_INTERVAL         600e6

// Sensor pin
#define ONE_WIRE_BUS      D7  // GPIO13

// LED pins
#define LED_PWR           D3
#define LED_WIFI          D2
#define LED_REG           D1

#define DEBUG             true

WiFiUDP udp;
mDNSResolver::Resolver resolver(udp);
WiFiClient tcp;
PubSubClient mqtt_client(tcp);

// prepare a 1-Wire bus connection
OneWire oneWire(ONE_WIRE_BUS); 
// prepare a Dallas temperature connection
DallasTemperature sensors(&oneWire);

// for getting VCC
ADC_MODE(ADC_VCC);

void setup() {
  // Disable WiFi antenna until we are ready for it
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);

  // turn on power LED
  lightOn(LED_PWR);

#if DEBUG
  // wait for serial to initialize
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { delay(500); }

  Serial.println();
  Serial.println();
  Serial.println("Initializing sensors");
#endif

  digitalWrite(ONE_WIRE_BUS, HIGH);
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP); // Mandatory to make it work on ESP8266

  // init sensors
  sensors.begin(); 

  // Give time to the sensor to settle
  delay(1500);
  float temp = readTemperature();
  float pwr = readPower();
#if DEBUG
  Serial.print("Temperature: ");
  Serial.println(temp);
#endif
  if (temp == -127) {
#if DEBUG
    Serial.println("Invalid reading, aborting.");
#endif
    finish();
  }

  // Connect to WiFi network
  connect();

  // Look for thermostat
  IPAddress ip = searchThermostat(THERMOSTAT_HOST);

  mqtt_client.setServer(ip, 1883);

#if DEBUG
  Serial.println("Connecting to broker");
#endif

  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  if (!mqtt_client.connect(clientId.c_str())) {
#if DEBUG
    Serial.println("Unable to connect to broker.");
    finish();
    return;
#endif
  }

#if DEBUG
  Serial.println("Sending reading to thermostat");
#endif

  String temp_payload = String("{") +
    "\"type\":\"temperature\"," +
    "\"validity\":" + int(SEND_INTERVAL/1000000) +"," +
    "\"unit\":\"celsius\"," +
    "\"value\":"+temp+"}";

  Serial.println(temp_payload);
  mqtt_client.publish(SENSOR_TOPIC, temp_payload.c_str());

#if DEBUG
  Serial.println("Sending status to thermostat");
#endif
/*
  http.begin("http://" + ip.toString() + ":" + THERMOSTAT_PORT + THERMOSTAT_API_SEND);
  http.addHeader("Content-Type", "application/json");

  message = String("{") +
    "\"sensor_id\": \""+SENSOR_ID+"\","
    "\"type\": \"power\"," +
    "\"unit\": \"volt\"," +
    "\"value\": \""+pwr+"\"}";
  Serial.println(message);

  httpCode = http.POST(message);
#if DEBUG
  Serial.print("Thermostat replied with ");
  Serial.println(httpCode);
#endif
  http.end();
*/

  mqtt_client.loop();
  mqtt_client.disconnect();

  finish();
}

void loop() {
}

void finish() {
  // Disconnect from WiFi
  WiFi.disconnect(true);
  delay(1);
  // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
  ESP.deepSleep(SEND_INTERVAL, WAKE_RF_DISABLED);
}

void connect() {
#if DEBUG
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
#endif

  WiFi.forceSleepWake();
  delay(1);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#if DEBUG
    Serial.print(".");
#endif
  }
#if DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
#endif

  lightOn(LED_WIFI);

  resolver.setLocalIP(WiFi.localIP());
}

IPAddress searchThermostat(const char* host) {
  #if DEBUG
  Serial.println("Searching for thermostat...");
#endif
  IPAddress ip = resolver.search(host);
#if DEBUG
  if(ip != INADDR_NONE) {
    Serial.print("Found thermostat: ");
    Serial.println(ip);
  } else {
    Serial.println("Thermostat not found!");
  }
#endif
  return ip;
}

float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

float readPower() {
  return (float) ESP.getVcc() / 1024;
}

void lightOn(int pin) {
#if DEBUG
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
#endif
}

void lightOff(int pin) {
#if DEBUG
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
#endif
}
