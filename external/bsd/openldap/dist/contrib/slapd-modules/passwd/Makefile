# OpenLDAP: pkg/ldap/contrib/slapd-modules/passwd/Makefile,v 1.2.2.2 2009/04/28 01:06:14 quanah Exp
CPPFLAGS+=-I../../../include -I../../../servers/slapd

all: kerberos.la netscape.la radius.la

kerberos.lo:	kerberos.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) -DHAVE_KRB5 -Wall -c $?

kerberos.la:	kerberos.lo
	$(LIBTOOL) --mode=link $(CC) -version-info 0:0:0 \
	-rpath $(PREFIX)/lib -module -o $@ $? -lkrb5

netscape.lo:	netscape.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) -Wall -c $?

netscape.la:	netscape.lo
	$(LIBTOOL) --mode=link $(CC) -version-info 0:0:0 \
	-rpath $(PREFIX)/lib -module -o $@ $? 

radius.lo:	radius.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) -Wall -c $?

radius.la:	radius.lo
	$(LIBTOOL) --mode=link $(CC) -version-info 0:0:0 \
	-rpath $(PREFIX)/lib -module -o $@ $? -lradius

clean:
	rm -f kerberos.lo kerberos.la
	rm -f netscape.lo netscape.la
	rm -f radius.lo radius.la

install: kerberos.la netscape.la radius.la
	mkdir -p $(PREFIX)/lib/openldap
	$(LIBTOOL) --mode=install cp kerberos.la $(PREFIX)/lib/openldap
	$(LIBTOOL) --mode=install cp netscape.la $(PREFIX)/lib/openldap
	$(LIBTOOL) --mode=install cp radius.la $(PREFIX)/lib/openldap
	$(LIBTOOL) --finish $(PREFIX)/lib
