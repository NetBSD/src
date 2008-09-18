CPPFLAGS=-I../../../include -I../../../servers/slapd
#LDFLAGS=-L/usr/local/openldap/lib
#LDFLAGS=-L/home/mszulczynski/autogroup/openldap/lib/
CC=gcc

all: autogroup.so

autogroup.so: autogroup.c
	$(CC) -shared -fPIC $(CPPFLAGS) $(LDFLAGS) -Wall -o $@ $?

clean:
	rm autogroup.so
