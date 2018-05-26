Remote HTTP sensors
===================

Remote HTTP sensor firmwares for ESP8266 using Arduino. For now only
temperature (via DS18S20 or similar sensor) is supported.

## Required libraries

* ESP8266 board support (installable via Arduino IDE Library Manager)
* [Modified OneWire+DallasTemperature](https://github.com/daniele-athome/OneWireNoResistor) (install the ESP8266 branch via ZIP file)

## Build and installation

Configure the sketch `remote-http-sensors.ino` by setting:

* WIFI_SSID: SSID of your WiFi network
* WIFI_PASSWORD: password of your WiFi network
* LISTEN_PORT: HTTP listen port
* THERMOSTAT_HOST: thermostat host
* THERMOSTAT_PORT: thermostat port
* THERMOSTAT_API: path to the thermostat API endpoint
* TEMP_SENSOR_ID: ID used by the temperature sensor to self-register to the thermostat

Connect your ESP8266-based device and upload the sketch using Arduino IDE.

## Sensors registration

Sensors will register itself at the configured thermostat automatically.

## Test the temperature sensor

Verify that the temperature sensor replies with a JSON temperature object:

```
$ curl http://SENSOR_ADDRESS:LISTEN_PORT/temperature
{"value":20.00,"unit":"celsius"}
```