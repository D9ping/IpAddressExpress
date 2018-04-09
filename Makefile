LDLIBS = -lcurl
CFLAGS = -O2

build:
	gcc publicipchangedetector.c $(LDLIBS) $(CFLAGS) -o publicipchangedetector 

