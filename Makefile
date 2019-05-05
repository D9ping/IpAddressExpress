LDLIBS = -lcurl -lsqlite3
CFLAGS = -O3 -std=c99 -fstack-protector-all
CDBFLAGS = -O0 -g -std=c99 -fstack-protector-all
build:
	gcc main.c db.c $(LDLIBS) $(CFLAGS) -o ipaddressexpress

debug:
	gcc main.c db.c $(LDLIBS) $(CDBFLAGS) -o ipaddressexpress
