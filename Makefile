LDLIBS = -lcurl -lsqlite3
CFLAGS = -O3 -std=c99

build:
	gcc main.c db.c $(LDLIBS) $(CFLAGS) -o ipaddressexpress

