OPENLDAP_SRC=/usr/local/src/openldap-2.4.6
CPPFLAGS+=-I${OPENLDAP_SRC}/include -I${OPENLDAP_SRC}/servers/slapd
LDFLAGS+=-L/usr/local/openldap-2.4.6
CC=gcc

all: addpartial-overlay.so

addpartial-overlay.so: addpartial-overlay.c
	$(CC) -shared $(CPPFLAGS) $(LDFLAGS) -Wall -o $@ $?

clean:
	rm addpartial-overlay.so
