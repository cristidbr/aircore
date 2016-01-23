# AirCore
AirCore is and ESP8266 based, open source WiFi module in a compact and lightweight footprint built to allow easier communication between your Arduino project and the Internet. 

##Firmware##
For now, the firmware is in development stage, so only the libraries I use are included. Once it's complete, I'll post the project code entirely. 

##Utilities##
All the tools I use are included in the Utilities folder of this project. For now, it's only the gzipping and minifying utility which is used to minify the webpages which will be served by the ESP8266. A manual for the utility is included in the index.htm file.

###Beware! This is not an IoT project###

Even if usage for such purposes is encouraged, the end goal of this module is to provide an easy method to connect your device to the Internet and obtain data from it. For people that are not network protocol experts, using an existing off the shelf WiFi module requires some complicated programming or the usage of libraries that don't always work, not to mention the time spent on programming. AirCore is removing the need of end users to program by moving the protocol implementation away in the device firmware of the ESP8266.

