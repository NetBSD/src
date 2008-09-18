# $OpenLDAP: pkg/ldap/contrib/slapd-modules/proxyOld/Makefile,v 1.1 2006/03/30 06:22:39 hyc Exp $
# Copyright 2005 Howard Chu, Symas Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.
#
LIBTOOL=../../../libtool
ROOT=../../../../..
rundir=$(LIB)
instdir=$(LIB)/openldap
CFLAGS=-g -O2
CC=$(C_CC)
INSTALL=../../../build/shtool install -c
LTVER=1:0:0

INCS=-I../../../include -I../../../servers/slapd -I ../../../../../opt/symas/include

LIBS=../../../libraries/libldap_r/libldap_r.la ../../../libraries/liblber/liblber.la

all:	proxyOld.la

proxyOld.lo:	proxyOld.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(DEFS) $(INCS) -c $?

proxyOld.la:	proxyOld.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) $(LDFLAGS) -version-info $(LTVER) \
	-rpath $(rundir) -module -o $@ $? $(LIBS) $(LIBEXTRAS)

install:
	-mkdir $(DESTDIR)$(instdir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 644 proxyOld.la $(DESTDIR)$(instdir)

clean:
	rm -rf *.o *.lo *.la .libs .libt

veryclean: clean
