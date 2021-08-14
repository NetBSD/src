# $OpenLDAP$
# Copyright 2014 David Coutadeur, Paris.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

LDAP_SRC = ../../..
LDAP_BUILD = $(LDAP_SRC)
LDAP_INC = -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB = $(LDAP_BUILD)/libraries/liblber/liblber.la $(LDAP_BUILD)/libraries/libldap/libldap.la

LIBTOOL = $(LDAP_BUILD)/libtool
INSTALL = /usr/bin/install
CC = gcc
OPT = -g -O2 -fpic

# To skip linking against CRACKLIB make CRACK=no
CRACK=yes
CRACKDEF_yes= -DCRACKLIB
CRACKDEF_no=

CRACKLIB_yes= -lcrack
CRACKLIB_no=

CRACKDEF=$(CRACKDEF_$(CRACK))
CRACKLIB=$(CRACKLIB_$(CRACK))

DEFS = -DDEBUG $(CRACKDEF)
# Define if using a config file:
# -DCONFIG_FILE="\"$(sysconfdir)/$(EXAMPLE)\""

INCS = $(LDAP_INC)
LIBS = $(LDAP_LIB)

PROGRAMS=ppm.so
LTVER = 0:0:0

LDAP_LIBS = -L$(LDAP_BUILD)/libraries/liblber/.libs -L$(LDAP_BUILD)/libraries/libldap/.libs -lldap -llber

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)
mandir = $(exec_prefix)/share/man
man5dir = $(mandir)/man5
etcdir = $(exec_prefix)/etc
sysconfdir = $(etcdir)$(ldap_subdir)

TEST=ppm_test
EXAMPLE=ppm.example
TESTS=./unit_tests.sh

all: 	ppm $(TEST)

$(TEST): ppm
	$(CC) $(CFLAGS) $(OPT) $(CPPFLAGS) $(LDFLAGS) $(INCS) $(LDAP_LIBS) -Wl,-rpath=. -o $(TEST) ppm_test.c $(PROGRAMS) $(LDAP_LIBS) $(CRACKLIB)

ppm.o:
	$(CC) $(CFLAGS) $(OPT) $(CPPFLAGS) $(DEFS) -c $(INCS) ppm.c

ppm: ppm.o
	$(CC) $(LDFLAGS) $(INCS) -shared -o $(PROGRAMS) ppm.o $(CRACKLIB)

install: ppm
	mkdir -p $(DESTDIR)$(moduledir)
	for p in $(PROGRAMS); do \
		$(LIBTOOL) --mode=install cp $$p $(DESTDIR)/$(moduledir) ; \
	done
	$(INSTALL) -m 644 $(EXAMPLE) $(DESTDIR)$(sysconfdir)/
#	$(INSTALL) -m 755 $(TEST) $(libdir)

.PHONY: clean

clean:
	$(RM) -f ppm.o $(PROGRAMS) ppm.lo $(TEST)
	$(RM) -rf .libs

test: ppm $(TEST)
	LDAP_SRC=$(LDAP_SRC) $(TESTS)


