To upload the firmware the Leonardo has to put into bootloader mode by pressing the reset button.
It will then appear at your PC as a serial device with a specific COM port, e.g. COM7.
Quickly before it leaves to bootloader due to timeout, run avrdude:

   avrdude -c avr109 -p atmega32u4 -P COM7 -b 57600 -U flash:w:arduino_leonardo.hex

where you have to adjust this line with the respective com port.
