Remote MQTT sensors
===================

Remote MQTT sensor firmwares for ESP-01 using Arduino. Designed to interact directly with a DHT11 MCU unit.

## Required libraries

* ESP8266 board support (installable via Arduino IDE Library Manager)
* [Arduino Client for MQTT](https://github.com/knolleary/pubsubclient) (installable via Arduino IDE Library Manager)
* [DHT sensor library by Adafruit](https://github.com/adafruit/DHT-sensor-library) (installable via Arduino IDE Library Manager)
* [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor) (installable via Arduino IDE Library Manager)

## Build and installation

Configure the sketch `remote-mqtt-sensors.ino` by setting:

* `WIFI_SSID`: SSID of your WiFi network
* `WIFI_PASSWORD`: password of your WiFi network
* `THERMOSTAT_HOST`: thermostat host
* `TEMP_SENSOR_ID`: ID used by the temperature sensor for identifying to the thermostat
* `SEND_INTERVAL`: if sensor is used in active mode, specify the interval in seconds in the form `SSe6` 
* `SENSOR_TOPIC`: MQTT topic to publish sensor data to, it shouldn't be changed

Connect your ESP-01 device and upload the sketch using Arduino IDE.

## Sensors registration

You will need to add the sensor previously through the thermostat web interface. Use these parameters:

* ID: same as TEMP\_SENSOR\_ID
* Protocol: `local`
* Address: `MQTT-REMOTE:`
* Type: `temperature`
* Icon: [choose an icon from FontAwesome](https://fontawesome.com/icons) and use only the icon name without any prefix
