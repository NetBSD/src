# $NetBSD: Makefile,v 1.7 2001/09/17 12:34:43 assar Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1
MKLINT= no

PROG= hpropd

SRCS= 	hpropd.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-lhdb					\
	-lkrb5					\
	-lkdb					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-L${VERS} -lvers			\
	-lroken					\
	-lcrypt

MAN=	hpropd.8

.include <bsd.prog.mk>
