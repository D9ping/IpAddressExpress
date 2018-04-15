LDLIBS = -lcurl
CFLAGS = -O3

build:
	gcc publicipchangedetector.c $(LDLIBS) $(CFLAGS) -o publicipchangedetector 

