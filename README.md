# MBed OS XBee API 2 library

This repository contains code for communicating between MBed OS and XBee devices using serial port communication. It is designed to work with XBee devices that are configured in API2 mode, and it is capable of sending and receiving data. At this time, it can parse any frame but can only send out Transmit Request frames (0x10). The code here is a work in progress, so more frame types/capabilities will be added eventually.

## Usage

Clone the *zigbee.cpp* and *zigbee.h* files. *main.cpp* provides an example that explains how to use the functions.

### License and contributions

The software is provided under Apache-2.0 license.

This project contains code from other projects. The original license text is included in those source files.
