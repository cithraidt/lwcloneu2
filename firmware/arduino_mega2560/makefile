
DIRS = m16u2 m2560

all:
	for i in $(DIRS); do make --directory=$$i clean; make --directory=$$i all; done

clean:
	for i in $(DIRS); do make --directory=$$i clean; done
