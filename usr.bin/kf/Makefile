# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 23:47:15 thorpej Exp $
DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/heimdal/appl/kf

WARNS?=  1
MKLINT= no

PROG= kf

SRCS=	kf.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/appl/kf		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

LDADD=	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-lroken \
	-lcrypt

NOMAN=

.include <bsd.prog.mk>
