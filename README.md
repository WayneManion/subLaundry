# subLaundry
MQTT Laundry Monitor

This was a pretty rudimentary setup for monitoring a laundry machine and a dryer for run status. I wrote [an article for The Tech Report about this setup](https://techreport.com/blog/31056/i-made-my-dumb-appliances-smarter-with-the-internet-of-things/). 

The INO file is for an ESP8266 microcontroller and a pair of LIS3DH gyroscopic sensors. The sensors report "tap" events to the MCU. The MCU counts the number of taps from each sensor in a 10-second period and sends that information over MQTT. 

The py file monitors the specified MQTT topic and fires off emails when the start and end of a cycle are detected. 

I have abandoned this approach and now use a smart plug with power monitoring to detect laundry machine cycles and a DS18B20 temperature probe strapped to my dryer vent tube to detect when the dryer is running. The smart plug and the ESP8266 with the temperature probe both run [Tasmota](https://tasmota.github.io/docs/). Their data is sent over MQTT. A group of [Home Assistant](https://home-assistant.io) automations monitor this incoming data and sent phone notifications at the start and end of machine cycles. 
