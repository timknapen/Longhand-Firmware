Longhand Firmware
=================

Firmware for the Longhand drawing machine.

This drawing machine operates by reading commands from a microSD card and executing them. The commands are stored in a .LHD or ""LongHandDrawing" file and are very simple graphic commands, mostly "lineTo" and "moveTo" commands.
The machine is fast, light and simple.


Built for Arduino Due with Arduino 1.5.2 or Arduino 1.5.6-r2

Arduino Due + 3 A4988 Pololu stepper motor driver carriers (aka "Stepstick")

A custom "Longhand Controller PCB" has been developed for this project. It is basically an interface between the Arduino DUE, a microSD connector, the motors and the stepsticks. The PCB will be released too, hopefully soon. 
For now, here are two pictures: a
[photo](https://flic.kr/p/njhb8j) and a
[screenshot](https://flic.kr/p/mDYHTF)

The old prototype layout looked like this: [http://www.flickr.com/photos/worldreceiver/8714276142/in/set-72157633417062123](http://www.flickr.com/photos/worldreceiver/8714276142/in/set-72157633417062123
)

[www.longhand.cc](http://www.longhand.cc)

