This repository contains the source code for my Arduino sketch used in my custom SmartLEDs project.
The code allows an Arduino Uno to read analog signals from a microphone connected to the A0 pin and a potentiometer connected to the A2 pin
which it will then use in conjunction with the arduinoFFT library to generate a new color to push into an array containing the
data for the LEDs. Meaning the LEDs will react to changes in the volume and pitch of sound that the microphone records.
Using the FastLED library, the data array is then used to update the output of the LEDs through a connection to the digital 9 pin output.
