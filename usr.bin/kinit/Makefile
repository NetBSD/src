# $NetBSD: Makefile,v 1.3 2000/08/03 04:02:31 assar Exp $
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

LDADD=						\
	-lkafs					\
	-lkrb5					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-lvers					\
	-lroken					\
	-lcrypt

.include <bsd.prog.mk>
