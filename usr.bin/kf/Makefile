# $NetBSD: Makefile,v 1.9 2001/12/12 01:48:45 tv Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/appl/kf

WARNS?=  1
NOLINT= # defined

PROG= kf

SRCS=	kf.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/appl/kf		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-L${VERS} -lvers \
	-lroken \
	-lcrypt

MAN1=	kf.1

.include <bsd.prog.mk>
