# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 23:47:14 thorpej Exp $
DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/heimdal/kpasswd

WARNS?=  1
MKLINT= no

PROG= kpasswd

SRCS= kpasswd.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kpasswd		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include		\
	-I${DESTDIR}/usr/include/openssl	\
	-DHAVE_CONFIG_H

LDADD=	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-lroken \
	-lcrypt

.include <bsd.prog.mk>
