# $NetBSD: Makefile,v 1.2.2.1 2001/04/05 23:25:28 he Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/hdb

WARNS?=  1
MKLINT= no

LIB=	hdb

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && ${PRINTOBJDIR}

hdb_err.c hdb_err.h: hdb_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/hdb/hdb_err.et

ERR_FILES = hdb_err.c

gen_files = asn1_Key.x asn1_Event.x asn1_HDBFlags.x asn1_hdb_entry.x asn1_Salt.x

DPSRCS=	hdb_asn1.h hdb_err.h

BUILT_SOURCES = $(gen_files:.x=.c) hdb_err.c

SRCS=					\
	    common.c			\
	    db.c			\
	    db3.c			\
	    hdb-ldap.c			\
	    hdb.c			\
	    keytab.c			\
	    mkey.c			\
	    ndbm.c			\
	    print.c			\
	    $(BUILT_SOURCES)

INCS=	hdb.h hdb_err.h hdb_asn1.h hdb-protos.h hdb-private.h

INCSDIR= /usr/include/krb5

ASN1COMPILE!=	cd ${.CURDIR}/../libasn1/asn1_compile && ${PRINTOBJDIR}

asn1_files hdb_asn1.hx: hdb.asn1
	${ASN1COMPILE}/asn1_compile ${DIST}/heimdal/lib/hdb/hdb.asn1 hdb_asn1

ASN1!=	cd ${.CURDIR}/../libasn1 && ${PRINTOBJDIR}
KRB5!=	cd ${.CURDIR}/../libkrb5 && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/hdb		\
	 -I${ASN1}				\
	 -I${KRB5}				\
	 -I${DIST}/heimdal/lib/krb5		\
	 -I${DIST}/openssl/crypto/rc4		\
	 -I${DIST}/openssl/crypto/des		\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

$(gen_files) hdb_asn1.h: asn1_files

hdb_asn1.h: hdb_asn1.hx
$(SRCS): hdb_err.h hdb_asn1.h

CLEANFILES = $(gen_files) $(BUILT_SOURCES) hdb_asn1.h hdb_asn1.hx asn1_files hdb_err.h

.include <bsd.lib.mk>

.SUFFIXES: .hx .x

.x.c:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

.hx.h:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

