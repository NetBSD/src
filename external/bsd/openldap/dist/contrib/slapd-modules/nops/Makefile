CPPFLAGS+=-I../../../include -I../../../servers/slapd

all: nops.so

nops.so: nops.c
	$(CC) -shared $(CPPFLAGS) -Wall -o $@ $?

clean:
	rm nops.so
