# OpenLDAP: pkg/ldap/contrib/slapd-modules/cloak/Makefile,v 1.2.2.1 2009/02/02 22:48:36 quanah Exp
CPPFLAGS+=-I../../../include -I../../../servers/slapd 
CPPFLAGS+=-DSLAPD_OVER_CLOAK=SLAPD_MOD_DYNAMIC
LIBS=-lldap_r -llber -lcrypto

all: cloak.la

cloak.lo:    cloak.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) -c $?

cloak.la:    cloak.lo
	$(LIBTOOL) --mode=link $(CC) -version-info 0:0:0 \
		   -rpath $(PREFIX)/lib -module -o $@ $? $(LIBS)

clean:
	rm cloak.lo cloak.la
