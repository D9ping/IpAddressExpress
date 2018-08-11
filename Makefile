LDLIBS = -lcurl
CFLAGS = -O3 -std=c99

build:
	gcc publicipchangedetector.c $(LDLIBS) $(CFLAGS) -o publicipchangedetector 

