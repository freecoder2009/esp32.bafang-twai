Application for Esp32 boards to communicate with Bafang e-bike controllers via CAN/TWAI protocol.

Various byte blocks can be changed, I for example needed a way to update assistance level and torque settings on my bafang motor. Codecs provided are based on the BESST Reference and CANable Pro implementations (see other projects below) and are able to convert between bytes/json of currently known byte blocks. Application can be extended with new codecs addressing byte blocks who's purpose is unknown at this time.

Esp32 behaves like a proxy between pc and CAN controller, accepting commands (read/write) from pc on the COM port in JSON format.
It translates JSON commands into CAN messages using provided codecs and then sends them over to the controller by CAN/TWAI protocol through the TJA1050 transceiver. Then it captures CAN responses, translates them to JSON and sends them back to pc through the COM port.

The project employs the Esp32 board and the TJA1050 transceiver board which combined can be an alternative to the CANable USB interface (https://canable.io/). The CANable interface though has UI support and thus it is a lot more user-friendly at this time.

The project was created using VStudioCode IDE with the PlatformIO plugin installed. I pushed project and plugin related config files to the repo as well.

Hardware needed:
1. Esp32 board with usb connector (7-8 euros on aliexpress)
2. TJA1050 transceiver board (1-2 euros on aliexpress)
3. Cable/connector to the bike's CAN network, similar to what CANable uses (https://kaspars.net/blog/bafang-canbus)

Software dependencies:
1. ArduinoJSON (https://arduinojson.org/)

Based on other Bafang CAN projects (codes, parameters, other values and constants):
1. Bafang Canable Pro (https://github.com/mdi-9/bafang_canable_pro/)
2. larssonwassen / bafang-can  (https://github.com/larssonwassen/bafang-can/)
3. BESST reference: https://github.com/kasparsd/besst-reference

DISCLAIMER: use this program at your own risk, I am not responsible for any damage you may do to your hardware while you use it.

Usage:

Currently you need to send JSON messages manually from a computer to the board, through its COM port; I used PlatformIO's serial monitor but any other serial communication tool can be used. Configure your tool to use baud rate 1000000 and CR (Carriage Return) as line ending.

I have included my platformio.ini file in the project so, after installing the plugin and connecting your Esp32 board I guess you can simply open the project in VSCode to have everything configured automatically for you.

First thing, build and upload the program to the board from within IDE, then you can start sending messages. I included a list of supported JSON messages and expected responses in res/examples.txt, I recommend you start with a read command message, like read Param0 to check that everything works fine. Just copy the "read param0" json from examples and paste it in the serial monitor then hit ENTER. Should be picked-up by the ESP32 and an answer should return instantly. 

The program will send back lots of debug messages in JSON format as well, current log level is DEBUG but you can change it to TRACE (at the top of the main.cpp file) if you want even more.

Please be aware that the program does not validate bytes you send in any way so be careful what you send. Always run a "read" command first, save what you get so you can restore it if something is wrong (copy/paste from the serial monitor); then modify it and send it back to be written. Always re-read the same parameter after write and see if value is the one you expect.

Do not turn on the motor before you are sure values are correct. I can say I had a bug in the initial development stages which messed-up all data in the controller but then I fixed the bug and successfully restored it so I am guessing nothing happens if you upload the wrong values IF you don't turn the motor on, but I am not sure about it, so it's best to always write correct values.

I am grateful to any feedback and improvement ideas. My intention was to create a UI as well for this app, however I am not sure when /if this will eventually happen.