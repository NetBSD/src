# $OpenLDAP: pkg/ldap/build/lib-shared.mk,v 1.22.2.3 2008/02/11 23:26:37 kurt Exp $
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2008 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##---------------------------------------------------------------------------
#
# Makefile Template for Shared Libraries
#

MKDEPFLAG = -l

.SUFFIXES: .c .o .lo

.c.lo:
	$(LTCOMPILE_LIB) $<

$(LIBRARY): version.lo
	$(LTLINK_LIB) -o $@ $(OBJS) version.lo $(LINK_LIBS)

Makefile: $(top_srcdir)/build/lib-shared.mk

