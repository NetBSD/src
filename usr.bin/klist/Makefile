# $NetBSD: Makefile,v 1.4 2000/08/03 22:47:42 assar Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1
MKLINT= no

PROG= klist

SRCS= klist.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kuser			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include		\
	-I${DESTDIR}/usr/include/openssl	\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && make print-objdir

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
