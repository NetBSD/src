# $NetBSD: Makefile,v 1.6 2000/12/30 14:54:40 sommerfeld Exp $
DIST=		${.CURDIR}/../../crypto/dist
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

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-L${VERS} -lvers \
	-lroken \
	-lcrypt

.include <bsd.prog.mk>
