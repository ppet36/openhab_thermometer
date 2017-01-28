# OpenHAB thermo & humidity meter

![alt](/images/done.jpg)

This repository contains Cadsoft's Eagle schematic and PCBs as well as Arduino firmware for simple temperature and humidity meter based on ESP8266-01 and AM2302 sensor. Meter is divided into two parts; indoor part with ESP and outdoor part with sensor.

For powering unit is used cheap DC/DC converter with LM2596 from eBay (previously set at 3.3V!!!); in my case is power supply shared with motorized venetian blinds in kitchen; they have 24VDC.

### Indoor part schematic
![alt](/eagle/temperature_sch.png)

### Outdoor part schematic
![alt](/eagle/temperature_sensor_sch.png)

Firmware can be programmed from Arduino IDE. For compilation is needed AM library from Adafruit which can be downloaded from https://learn.adafruit.com/dht/downloads. WIFI access point and network parameters can be set in source code. After device startup is on his IP address available HTTP server which provides measured temperature and humidity values as REST services. Via HTTP server is also available simple configuration page.

PCB boards is designed as single-sided. Indoor part of thermometer is placed in rectangular electrical ducting. For outdoor part i created simple model, which is available in openscad directory.

### OpenHAB
OpenHAB definition is also very simple. You need to edit your items and sitemap files. In items file is needed to add definition of services (replace IP address with IP address of your device):
```
Number outdoorTemperature "Outdoor temperature [%.1f °C]" <temperature> { http="<[http://192.168.128.205/temperature:15000:JS(doublevalue.js)]" }
Number outdoorHumidity "Outdoor humidity [%.1f %%]" <water> { http="<[http://192.168.128.205/humidity:15000:JS(doublevalue.js)]" }
```
Finally can be measured values added to your sitemap file:
```    
Text item=outdoorTemperature
Text item=outdoorHumidity
```
After successfull configuration are available measured values on OpenHAB page:
![alt](/images/mobile.jpg)

## Some project images
![alt](/images/dps.jpg)
![alt](/images/dps_sensor.jpg)
![alt](/images/mounted.jpg)

