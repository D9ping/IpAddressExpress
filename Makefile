LDLIBS = -lcurl -lsqlite3
CFLAGS = -O3 -std=c99 -fstack-protector-all

build:
	gcc main.c db.c $(LDLIBS) $(CFLAGS) -o ipaddressexpress

