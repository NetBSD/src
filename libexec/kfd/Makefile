# $NetBSD: Makefile,v 1.5 2000/08/03 22:47:40 assar Exp $
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

VERS!=	cd ${.CURDIR}/../../lib/libvers && make print-objdir

LDADD=	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-L${VERS} -lvers \
	-lroken \
	-lcrypt

MAN=	kfd.8

.include <bsd.prog.mk>
