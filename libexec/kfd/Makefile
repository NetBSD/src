# $NetBSD: Makefile,v 1.6 2000/10/26 05:52:55 garbled Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/appl/kf

WARNS?=  1
MKLINT= no

PROG= kfd

SRCS= 	kfd.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/appl/kf		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${MAKE} print-objdir

LDADD=	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-L${VERS} -lvers \
	-lroken \
	-lcrypt

MAN=	kfd.8

.include <bsd.prog.mk>
