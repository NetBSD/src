# $NetBSD: Makefile,v 1.2 2000/06/16 22:46:46 thorpej Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/hdb

WARNS?=  1
MKLINT= no

LIB=	hdb

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && make print-objdir

hdb_err.c hdb_err.h: hdb_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/hdb/hdb_err.et

ERR_FILES = hdb_err.c

gen_files = asn1_Key.x asn1_Event.x asn1_HDBFlags.x asn1_hdb_entry.x asn1_Salt.x

DPSRCS=	hdb_err.h

BUILT_SOURCES = $(gen_files:.x=.c) hdb_err.c

SRCS=	\
	    keytab.c			\
	    hdb.c			\
	    common.c			\
	    db.c			\
	    ndbm.c			\
	    print.c			\
	    $(BUILT_SOURCES)

INCS=	hdb.h hdb_err.h hdb_asn1.h hdb-protos.h hdb-private.h

INCSDIR= /usr/include/krb5

$(gen_files) asn1.h: asn1_files

ASN1COMPILE!=	cd ${.CURDIR}/../libasn1/asn1_compile && ${MAKE} print-objdir

asn1_files: hdb.asn1
	${ASN1COMPILE}/asn1_compile ${DIST}/heimdal/lib/hdb/hdb.asn1 hdb_asn1

ASN1!=	cd ${.CURDIR}/../libasn1 && ${MAKE} print-objdir
KRB5!=	cd ${.CURDIR}/../libkrb5 && ${MAKE} print-objdir

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
$(SRCS:.c=.o): hdb_err.h hdb_asn1.h

CLEANFILES = $(gen_files) $(BUILT_SOURCES) hdb_asn1.h hdb_asn1.hx asn1_files hdb_err.h

.include <bsd.lib.mk>

.SUFFIXES: .hx .x

.x.c:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

.hx.h:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

