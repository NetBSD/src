# $NetBSD: Makefile,v 1.10 2001/12/31 19:02:49 thorpej Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1

PROG= kdestroy

SRCS= kdestroy.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kuser			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include/openssl	\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=						\
	-lkafs					\
	-lkrb5					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-L${VERS} -lvers			\
	-lroken					\
	-lcrypt

.include <bsd.prog.mk>
