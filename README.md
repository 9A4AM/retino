Retino
===
The poor man's radiosonde receiver

What is it
---
An alternative firmware for the HC-14 LoRa modules for receiving and decoding meteorological radiosondes. To be used in conjunction with a USB TTL serial adapter

Currently able to receive the following radiosondes:
- Vaisala RS41[^1]
- Meteomodem M10
- Meteomodem M20
- Graw DFM09

[^1]: Receive only, no decoding. Raw packet transmitted to host

How to connect to the USB TTL serial adapter
---
|HC-14|adapter|
|---|---|
|VCC|VCC|
|GND|GND|
|TX|RX|
|RX|TX|
|KEY|DTR|

Input "protocol"
---
Line oriented
- ? request settings
- ttttttffffff 6 characters that represent the sonde type, right padded with blanks, followed by 6 charactes indicating the requested frequency in kHz

Output "protocol"
---
Line oriented, first character specifies record type:
- D: decoded data. Followed by key:value pairs separated by commas
- P: raw packet. Followed by data in hex. Data is already "massaged" (manchester decoded and dewhitened)
- #: response to confirm new settings. Followed by sonde type and  frequency in Hz separated by '@'

Reverse engineering of the HC-14 module
---
- Processor: Nuvoton MS51XC0BE
- Radio module: Semtech SX1278
- [PG2179TB](https://www.mouser.com/datasheet/2/286/nec_cel_upg2179tb-1186632.pdf) RF switch

![HC-14 module schematic](HC-14.svg "HC-14 module schematic")

Other connections:
|Function|Pin|
|---|---|
|SX1278 DIO0|P0.4|
|SX1278 DIO1|P0.3|
|SX1278 NRESET|P0.5|
|SX1278 MISO|P0.1/SPI0_MISO|
|SX1278 MOSI|P0.0/SPI0_MOSI|
|SX1278 SCK|P1.0/SPI0_CLK|
|SX1278 NSS|P1.2|
|SX1278 RXTX/RFMOD|P1.3|
|PG2179TB vcont2|P1.7|
|PG2179TB vcont1|P1.5|

Userful links
---
- https://github.com/OpenNuvoton/MS51_BSP
- https://wolles-elektronikkiste.de/en/hc-14-the-simple-lora-module
- https://github.com/misaz/Nuvoton8051ProgrammingLib
