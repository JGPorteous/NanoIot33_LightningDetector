# NanoIot33_LightningDetector
Lightning Detector using Arduino IOT Nano 33

## Parts used
Arduino Nano IOT 33
SparkFun AS3935 Lightning Detector
DHT11 (Temperature and Humidity)

## Architecture
The Arduino will detect lightning and collect temperature and humidity.

These values or events, are then sent to a MQTT server which Home Assistant is monitoring.

SvxLink server is connected to the repeater, which serves as an EchoLink node. Data is collected from MQTT and used to broadcast lightning and weather details.

## Links
https://store.arduino.cc/usa/nano-33-iot

https://www.sparkfun.com/products/retired/15276

https://www.adafruit.com/product/386

http://www.echolink.org/

https://www.svxlink.org/

