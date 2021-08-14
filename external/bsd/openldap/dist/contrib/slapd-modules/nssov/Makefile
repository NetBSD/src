# $OpenLDAP$
# This work is part of OpenLDAP Software <http://www.openldap.org/>.
#
# Copyright 2008-2021 The OpenLDAP Foundation.
# Portions Copyright 2008 Howard Chu, Symas Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

# Path to the OpenLDAP source tree
LDAP_SRC=../../..

# Path to the OpenLDAP object tree - same as above unless
# you're doing out-of-tree builds.
LDAP_BUILD=$(LDAP_SRC)

LDAP_INC = -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB = $(LDAP_BUILD)/libraries/libldap/libldap.la \
	$(LDAP_BUILD)/libraries/liblber/liblber.la

NLDAPD_INC=-Inss-pam-ldapd

LIBTOOL = $(LDAP_BUILD)/libtool
INSTALL = /usr/bin/install
OPT = -g -O2
CC = gcc
DEFS =
INCS = $(LDAP_INC) $(NLDAPD_INC)
LIBS = $(LDAP_LIB)

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)
sysconfdir = $(prefix)/etc$(ldap_subdir)
schemadir = $(sysconfdir)/schema
mandir = $(exec_prefix)/share/man
man5dir = $(mandir)/man5

all:	nssov.la

XOBJS = tio.lo

OBJS = alias.lo ether.lo group.lo host.lo netgroup.lo network.lo \
	nssov.lo passwd.lo protocol.lo rpc.lo service.lo shadow.lo pam.lo

MANPAGES = slapo-nssov.5

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(OPT) $(CPPFLAGS) $(DEFS) $(INCS) -c $<

tio.lo:	nss-pam-ldapd/tio.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(OPT) $(CPPFLAGS) $(DEFS) $(INCS) -c $?

$(OBJS):	nssov.h

nssov.la:	$(OBJS) $(XOBJS)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -version-info 0:0:0 \
	-rpath $(moduledir) -module -o $@ $(OBJS) $(XOBJS) $(LIBS)

install: install-lib install-man FORCE

install-lib: nssov.la
	mkdir -p $(DESTDIR)$(moduledir)
	$(LIBTOOL) --mode=install cp nssov.la $(DESTDIR)$(moduledir)
	cp ldapns.schema $(DESTDIR)$(schemadir)

install-man: $(MANPAGES)
	mkdir -p  $(DESTDIR)$(man5dir)
	$(INSTALL) -m 644 $(MANPAGES) $(DESTDIR)$(man5dir)

FORCE:

clean:
	rm -f *.*o *.la .libs/*
	rm -rf .libs
