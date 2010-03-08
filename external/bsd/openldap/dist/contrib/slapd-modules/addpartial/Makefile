# OpenLDAP: pkg/ldap/contrib/slapd-modules/addpartial/Makefile,v 1.1.2.4 2009/01/21 00:18:19 quanah Exp
OPENLDAP_SRC=../../..
OPENLDAP_BLD=../../..
CPPFLAGS+=-I${OPENLDAP_SRC}/include -I${OPENLDAP_SRC}/servers/slapd -I${OPENLDAP_BLD}/include
CC=gcc

all: addpartial-overlay.so

addpartial-overlay.so: addpartial-overlay.c
	$(CC) -shared $(CPPFLAGS) $(LDFLAGS) -Wall -o $@ $?

clean:
	rm addpartial-overlay.so
