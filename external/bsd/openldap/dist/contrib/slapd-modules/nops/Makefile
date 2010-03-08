# OpenLDAP: pkg/ldap/contrib/slapd-modules/nops/Makefile,v 1.1.2.4 2009/04/27 23:35:48 quanah Exp
CPPFLAGS+=-I../../../include -I../../../servers/slapd 
CPPFLAGS+=-DSLAPD_OVER_NOPS=SLAPD_MOD_DYNAMIC
LIBS=-L$(PREFIX)/lib -lldap_r -llber -lcrypto

all: nops.la

nops.lo:    nops.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) -c $?

nops.la:    nops.lo
	$(LIBTOOL) --mode=link $(CC) -version-info 0:0:0 \
		   -rpath $(PREFIX)/lib -module -o $@ $? $(LIBS)

clean:
	rm -f nops.lo nops.la

install: nops.la
	mkdir -p $(PREFIX)/lib/openldap 
	mkdir -p $(PREFIX)/man/man5
	$(LIBTOOL) --mode=install cp nops.la $(PREFIX)/lib/openldap
	$(LIBTOOL) --finish $(PREFIX)/lib
	cp nops.5 $(PREFIX)/man/man5
