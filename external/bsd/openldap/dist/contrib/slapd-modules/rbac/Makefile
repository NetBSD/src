# $OpenLDAP$

LDAP_SRC = ../../..
LDAP_BUILD = $(LDAP_SRC)
LDAP_INC = -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB = $(LDAP_BUILD)/libraries/libldap/libldap.la \
	$(LDAP_BUILD)/libraries/liblber/liblber.la

PLAT = UNIX
NT_LIB = -L$(LDAP_BUILD)/servers/slapd -lslapd
NT_LDFLAGS = -no-undefined -avoid-version
UNIX_LDFLAGS = -version-info $(LTVER)

LIBTOOL = $(LDAP_BUILD)/libtool
INSTALL = /usr/bin/install
CC = gcc
OPT = -g -O2
DEFS = -DSLAPD_OVER_RBAC=SLAPD_MOD_DYNAMIC
INCS = $(LDAP_INC)
LIBS = $($(PLAT)_LIB) $(LDAP_LIB)
LD_FLAGS = $(LDFLAGS) $($(PLAT)_LDFLAGS) -rpath $(moduledir) -module

PROGRAMS = rbac.la
MANPAGES = slapo-rbac.5

LTVER = 0:0:0

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)
mandir = $(exec_prefix)/share/man
man5dir = $(mandir)/man5

SRCS = rbac.c rbacperm.c rbacsess.c rbacuser.c rbacreq.c rbacaudit.c init.c rbacacl.c util.c jts.c
OBJS = $(patsubst %.c,%.o,$(SRCS))
LOBJS = $(patsubst %.c,%.lo,$(SRCS))

.SUFFIXES: .c .lo

%.lo: %.c rbac.h
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(OPT) $(CPPFLAGS) $(DEFS) $(INCS) -c $<

all: $(PROGRAMS)

rbac.la: $(LOBJS)
	$(LIBTOOL) --mode=link $(CC) $(LD_FLAGS) -o $@ $? $(LIBS)

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

