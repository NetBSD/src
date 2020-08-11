# $OpenLDAP$

LDAP_SRC = ../../../..
LDAP_BUILD = ../../../..
LDAP_INC = -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB = $(LDAP_BUILD)/libraries/libldap_r/libldap_r.la \
	$(LDAP_BUILD)/libraries/liblber/liblber.la

LIBTOOL = $(LDAP_BUILD)/libtool
INSTALL = /usr/bin/install
CC = gcc
OPT = -g -O2 -Wall
#DEFS = -DSLAPD_ARGON2_DEBUG

INCS = $(LDAP_INC)
LIBS = $(LDAP_LIB)

implementation = sodium

ifeq ($(implementation),argon2)
LIBS += -largon2
DEFS += -DSLAPD_ARGON2_USE_ARGON2
else ifeq ($(implementation),sodium)
LIBS += -lsodium
DEFS += -DSLAPD_ARGON2_USE_SODIUM
else
$(error Unsupported implementation $(implementation))
endif

PROGRAMS = pw-argon2.la
MANPAGES = slapd-pw-argon2.5
LTVER = 0:0:0

#prefix=/usr/local
prefix=`grep -e "^prefix =" $(LDAP_BUILD)/Makefile | cut -d= -f2`

exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)
mandir = $(exec_prefix)/share/man
man5dir = $(mandir)/man5

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $<

all:		$(PROGRAMS)

pw-argon2.la: pw-argon2.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info $(LTVER) \
	-rpath $(moduledir) -module -o $@ $? $(LIBS)

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
