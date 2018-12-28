Remote MQTT sensors
===================

Remote MQTT sensor firmwares for ESP8266 using Arduino. For now only
temperature (via DS18B20 or similar sensor) is supported.

## Required libraries

* ESP8266 board support (installable via Arduino IDE Library Manager)
* [Modified OneWire+DallasTemperature](https://github.com/daniele-athome/OneWireNoResistor) (install the ESP8266 branch via ZIP file)
* [Modified mDNSResolver](https://github.com/daniele-athome/mDNSResolver) (follow instructions in README)

## Build and installation

Configure the sketch `remote-mqtt-sensors.ino` by setting:

* `WIFI_SSID`: SSID of your WiFi network
* `WIFI_PASSWORD`: password of your WiFi network
* `THERMOSTAT_HOST`: thermostat host
* `TEMP_SENSOR_ID`: ID used by the temperature sensor for identifying to the thermostat
* `SEND_INTERVAL`: if sensor is used in active mode, specify the interval in seconds in the form `SSe6` 
* `SENSOR_TOPIC`: MQTT topic to publish sensor data to, it shouldn't be changed

> TODO explain LED usage

Connect your ESP8266 device and upload the sketch using Arduino IDE.

## Sensors registration

You will need to add the sensor previously through the thermostat web interface. Use these parameters:

* ID: same as TEMP_SENSOR_ID
* Protocol: `local`
* Address: `MQTT-LOCAL:`
* Type: `temperature`
* Icon: [choose an icon from FontAwesome](https://fontawesome.com/icons) and use only the icon name without any prefix
