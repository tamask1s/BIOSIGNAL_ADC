ADS1293Board | ESP32  
-------------------  
+5V          | VIN  
GND          | GND  
SCLK         | D14  
SDI(MOSI)    | D13  
SDO(MISO)    | D12  
CSB          | D15  
WCT          | X  
ALAB         | D2  
DRDB         | D4  

libs to install: ESPAsyncWebSrv  

bluetooth, wifi, and accesspoint settings are all available, but only either bluetooth or wifi can be used at a time.  

switching between wifi and bluetooth modes can be done with issuing the following commands:  

```json
{"requests":[{"command":"set_bluetooth_settings","parameters":{"device_name":"ESP32-BT-Slave", "reconnect":true}}]}
{"requests":[{"command":"set_wifi_settings","parameters":{"ssid":"TP-LINK_EC830C","password":"02947297", "reconnect":true}}]}
```  

commands can be sent on serial usb, or on the active wireless interface  
ap mode is automatically activated if wifi is enabled. ap mode is working even if the device cannot connect to a router.  

ip:port on router network: 192.168.1.184:80  
ip:port on sof ap mode: 192.168.4.1:80  
bluetooth device name: ESP32-BT-Slave  
