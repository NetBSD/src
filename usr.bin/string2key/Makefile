# $NetBSD: Makefile,v 1.6 2000/12/30 14:54:41 sommerfeld Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1
MKLINT= no

PROG= string2key

SRCS= string2key.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
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

NOMAN=

.include <bsd.prog.mk>
