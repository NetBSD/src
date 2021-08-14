# $OpenLDAP$

LDAP_SRC = ../../..
LDAP_BUILD = $(LDAP_SRC)
LDAP_INC = -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB = $(LDAP_BUILD)/libraries/libldap/libldap.la \
	$(LDAP_BUILD)/libraries/liblber/liblber.la

LIBTOOL = $(LDAP_BUILD)/libtool
INSTALL = /usr/bin/install
CC = gcc
OPT = -g -O2
DEFS = 
INCS = $(LDAP_INC)
LIBS = $(LDAP_LIB)

PROGRAMS = pw-kerberos.la pw-netscape.la pw-radius.la pw-apr1.la
MANPAGES = slapd-pw-radius.5
LTVER = 0:0:0

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)
mandir = $(exec_prefix)/share/man
man5dir = $(mandir)/man5

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(OPT) $(CPPFLAGS) $(DEFS) $(INCS) -c $<

all: $(PROGRAMS)

pw-kerberos.la:	kerberos.lo
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -version-info $(LTVER) \
	-rpath $(moduledir) -module -o $@ $? -lkrb5

pw-netscape.la:	netscape.lo
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -version-info $(LTVER) \
	-rpath $(moduledir) -module -o $@ $?

pw-radius.la:	radius.lo
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -version-info $(LTVER) \
	-rpath $(moduledir) -module -o $@ $? -lradius

pw-apr1.la:	apr1.lo
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -version-info $(LTVER) \
	-rpath $(moduledir) -module -o $@ $?

clean:
	rm -rf *.o *.lo *.la .libs

install: install-lib install-man FORCE

install-lib: $(PROGRAMS)
	mkdir -p $(DESTDIR)$(moduledir)
	for p in $(PROGRAMS) ; do \
		$(LIBTOOL) --mode=install cp $$p $(DESTDIR)$(moduledir) ; \
	done

install-man: $(MANPAGES)
	mkdir -p  $(DESTDIR)$(man5dir)
	$(INSTALL) -m 644 $(MANPAGES) $(DESTDIR)$(man5dir)

FORCE:

