# $NetBSD: Makefile,v 1.5 2000/10/26 05:57:01 garbled Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1
MKLINT= no

PROG= kinit

SRCS= kinit.c kinit_options.c

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/kuser		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${DESTDIR}/usr/include/kerberosIV	\
	 -I${DESTDIR}/usr/include/krb5		\
	 -I${DESTDIR}/usr/include		\
	 -I${DESTDIR}/usr/include/openssl	\
	 -DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${MAKE} print-objdir

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
