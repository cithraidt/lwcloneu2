
Program ATmega2560:
-------------------
avrdude -c wiring -p atmega2560 -P com20 -b 115200 -v -U flash:w:m2560/arduino_mega2560__m2560.hex:i


Program ATmega8u2 / ATmega16u2
------------------------------
Use the tool "Flip" from Atmel and upload "arduino_mega2560__m16u2.hex" or 
to revert to Arduino, upload "Arduino-usbserial-atmega16u2-Mega2560-Rev3.hex" from Arduino installation folder "arduino-root\hardware\arduino\firmwares\atmegaxxu2\arduino-usbserial"
