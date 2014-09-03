
DIRS = arduino_mega2560 arduino_leonardo arduino_uno breakout_32u2 arduino_promicro

all:
	for i in $(DIRS); do make --directory=$$i clean; make --directory=$$i all; done

clean:
	for i in $(DIRS); do make --directory=$$i clean; done
