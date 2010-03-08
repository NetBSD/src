# OpenLDAP: pkg/ldap/contrib/slapd-modules/allowed/Makefile,v 1.1.2.2 2009/08/25 23:39:37 quanah Exp
# This work is part of OpenLDAP Software <http://www.openldap.org/>.
#
# Copyright 1998-2009 The OpenLDAP Foundation.
# Copyright 2004 Howard Chu, Symas Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

PREFIX=/opt/openldap-HEAD

LIBTOOL=../../../libtool
OPT=-g -O2
CC=gcc

DEFS=-DSLAPD_OVER_ALLOWED=SLAPD_MOD_DYNAMIC

LDAP_INC=-I../../../include -I../../../servers/slapd
INCS=$(LDAP_INC)

LDAP_LIB=-lldap_r -llber -L../../../lib
LDAP_LIB=
LIBS=$(LDAP_LIB)

all:	allowed.la


allowed.lo:	allowed.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $?

allowed.la:	allowed.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info 0:0:0 \
	-rpath $(PREFIX)/lib -module -o $@ $? $(LIBS)

clean:
	rm -f allowed.o allowed.lo allowed.la

install: allowed.la
	mkdir -p $(PREFIX)/libexec/openldap
	$(LIBTOOL) --mode=install cp allowed.la $(PREFIX)/libexec/openldap
	$(LIBTOOL) --finish $(PREFIX)/libexec/openldap
