# $NetBSD: Makefile,v 1.9 2001/12/12 01:48:47 tv Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1
NOLINT= # defined

PROG= string2key
MAN=  string2key.8

SRCS= string2key.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include/openssl	\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-L${VERS} -lvers \
	-lroken \
	-lcrypt

.include <bsd.prog.mk>
