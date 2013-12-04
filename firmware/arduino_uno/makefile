
DIRS = m328 m8u2

all:
	for i in $(DIRS); do make --directory=$$i clean; make --directory=$$i all; done

clean:
	for i in $(DIRS); do make --directory=$$i clean; done
