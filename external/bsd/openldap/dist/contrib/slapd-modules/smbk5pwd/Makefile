# $OpenLDAP: pkg/ldap/contrib/slapd-modules/smbk5pwd/Makefile,v 1.1 2004/04/02 11:06:38 hyc Exp $
# Copyright 2004 Howard Chu, Symas Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

LIBTOOL=../../../libtool
OPT=-g -O2
CC=gcc

# Omit DO_KRB5 or DO_SAMBA if you don't want to support it.
DEFS=-DDO_KRB5 -DDO_SAMBA

HEIMDAL_INC=-I/usr/heimdal/include
SSL_INC=
LDAP_INC=-I../../../include -I../../../servers/slapd
INCS=$(LDAP_INC) $(HEIMDAL_INC) $(SSL_INC)

HEIMDAL_LIB=-L/usr/heimdal/lib -lkrb5 -lkadm5srv
SSL_LIB=-lcrypto
LDAP_LIB=-lldap_r -llber
LIBS=$(LDAP_LIB) $(HEIMDAL_LIB) $(SSL_LIB)

all:	smbk5pwd.la


smbk5pwd.lo:	smbk5pwd.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $?

smbk5pwd.la:	smbk5pwd.lo
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info 0:0:0 \
	-rpath /usr/local/libexec/openldap -module -o $@ $? $(LIBS)
