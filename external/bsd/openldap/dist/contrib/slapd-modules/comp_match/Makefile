# $OpenLDAP: pkg/ldap/contrib/slapd-modules/comp_match/Makefile,v 1.11.2.3 2008/02/11 23:26:38 kurt Exp $
# This work is part of OpenLDAP Software <http://www.openldap.org/>.
#
# Copyright 2003-2008 The OpenLDAP Foundation.
# Portions Copyright 2004 by IBM Corporation.
# All rights reserved.

# Copyright 2004 Sang Seok Lim, IBM Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

topsrcdir = ../../..
snaccdir = ../$(topsrcdir)/snacc
openssldir = /usr/local/include/openssl

LIBTOOL=$(topsrcdir)/libtool
OPT=-g -O2 -DLDAP_COMPONENT
CC=gcc

SNACC_INC=-I$(snaccdir) -I$(snaccdir)/c-lib/inc
LDAP_INC=-I$(topsrcdir)/include -I$(topsrcdir)/servers/slapd -I$(topbuilddir)/include
OPENSSL_INC=-I$(openssldir)
INCS=$(LDAP_INC) $(SNACC_INC) $(OPENSSL_INC)

SNACC_LIB=$(snaccdir)/c-lib/libcasn1.a
SSL_LIB=/usr/local/lib/lcrypto

LIBS=$(LDAP_LIB) $(SNACC_LIB) $(SSL_LIB)

all:	compmatch.la


componentlib.lo:	componentlib.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(INCS) -c $?

certificate.lo:	certificate.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(INCS) -c $?

crl.lo:	crl.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(INCS) -c $?

authorityKeyIdentifier.lo:	authorityKeyIdentifier.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(INCS) -c $?

asn_to_syn_mr.lo:	asn_to_syn_mr.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(INCS) -c $?

init.lo:	init.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(INCS) -c $?

compmatch.la:	componentlib.lo init.lo certificate.lo asn_to_syn_mr.lo authorityKeyIdentifier.lo crl.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) $(LIBS) -version-info 0:0:0 \
	-rpath /usr/local/libexec/openldap -module -o $@ $? $(LIBS)

clean:
	\rm compmatch.la componentlib.lo certificate.lo asn_to_syn_mr.lo authorityKeyIdentifier.lo crl.lo\
		init.o init.lo componentlib.o certificate.o asn_to_syn_mr.o authorityKeyIdentifier.o crl.o
install:
	cp -r .libs $(topsrcdir)/tests/data/comp_libs
	cp compmatch.la  $(topsrcdir)/tests/data/comp_libs
