# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 23:47:15 thorpej Exp $
DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1
MKLINT= no

PROG= kdestroy

SRCS= kdestroy.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kuser			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include		\
	-I${DESTDIR}/usr/include/openssl	\
	-DHAVE_CONFIG_H

LDADD=						\
	-lkafs					\
	-lkrb5					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-lroken					\
	-lcrypt

.include <bsd.prog.mk>
