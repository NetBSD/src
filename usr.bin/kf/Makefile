# $NetBSD: Makefile,v 1.4 2000/08/03 04:02:30 assar Exp $
DIST=		${.CURDIR}/../../crypto/dist
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
	-lvers \
	-lroken \
	-lcrypt

MAN1=	kf.1

.include <bsd.prog.mk>
