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
CC = gcc
OPT = -g -O2
DEFS = 
INCS = $(LDAP_INC)
LIBS = $($(PLAT)_LIB) $(LDAP_LIB)
LD_FLAGS = $(LDFLAGS) $($(PLAT)_LDFLAGS) -rpath $(moduledir) -module

PROGRAMS = addpartial.la
LTVER = 0:0:0

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(OPT) $(CPPFLAGS) $(DEFS) $(INCS) -c $<

all: $(PROGRAMS)

addpartial.la: addpartial-overlay.lo
	$(LIBTOOL) --mode=link $(CC) $(LD_FLAGS) -o $@ $? $(LIBS)

clean:
	rm -rf *.o *.lo *.la .libs

install: $(PROGRAMS)
	mkdir -p $(DESTDIR)$(moduledir)
	for p in $(PROGRAMS) ; do \
		$(LIBTOOL) --mode=install cp $$p $(DESTDIR)$(moduledir) ; \
	done

