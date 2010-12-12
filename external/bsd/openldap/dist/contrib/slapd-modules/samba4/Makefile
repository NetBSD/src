# OpenLDAP: pkg/ldap/contrib/slapd-modules/samba4/Makefile,v 1.3.2.3 2010/06/10 17:44:02 quanah Exp
# This work is part of OpenLDAP Software <http://www.openldap.org/>.
#
# Copyright 1998-2010 The OpenLDAP Foundation.
# Copyright 2004 Howard Chu, Symas Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

LDAP_SRC=../../..
# craft according to your installation
LDAP_BUILD=../../..

LIBTOOL=$(LDAP_BUILD)/libtool
OPT=-g -O2
CC=gcc

DEFS=-DSLAPD_OVER_RDNVAL=2 -DSLAPD_OVER_PGUID=2 -DSLAPD_OVER_VERNUM=2

LDAP_INC=-I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd \
	-I$(LDAP_BUILD)/include
INCS=$(LDAP_INC)

LDAP_LIB=-lldap_r -llber
LIBS=$(LDAP_LIB)

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)

PROGRAMS = pguid.la rdnval.la vernum.la

all:	$(PROGRAMS)

pguid.lo:	pguid.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $?

pguid.la:	pguid.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info 0:0:0 \
	-rpath $(moduledir) -module -o $@ $? $(LIBS)

rdnval.lo:	rdnval.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $?

rdnval.la:	rdnval.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info 0:0:0 \
	-rpath $(moduledir) -module -o $@ $? $(LIBS)

vernum.lo:	vernum.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $?

vernum.la:	vernum.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info 0:0:0 \
	-rpath $(moduledir) -module -o $@ $? $(LIBS)

clean:
	rm -f \
		pguid.o pguid.lo pguid.la \
		rdnval.o rdnval.lo rdnval.la \
		vernum.o vernum.lo vernum.la

install: $(PROGRAMS)
	mkdir -p $(DESTDIR)$(moduledir)
	for p in $(PROGRAMS) ; do \
		$(LIBTOOL) --mode=install cp $$p $(DESTDIR)$(moduledir) ; \
	done

