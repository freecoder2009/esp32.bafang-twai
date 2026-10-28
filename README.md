Application for esp32 board to communicate with Bafang e-bike CAN controllers via json.
Esp32 behaves like a proxy between pc and the controller, accepting commands (read/write) on the serial port in json format.
It translates them into CAN requests using provided codecs and then sends them over to the controller by TWAI protocol.
The project employs ESP32 board and TJA1050 transceiver.

Hardware needed:
1. ESP32 board with usb connector
2. TJA1050 transceiver

Software dependencies:
1. ArduinoJSON (https://arduinojson.org/)

Based on other Bafang CAN projects (codes, parameters, other values and constants):
1. Bafang Canable Pro (https://github.com/mdi-9/bafang_canable_pro/)
2. larssonwassen / bafang-can  (https://github.com/larssonwassen/bafang-can/)
3. BESST reference: https://github.com/kasparsd/besst-reference
