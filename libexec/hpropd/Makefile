# $NetBSD: Makefile,v 1.2 2000/06/16 23:41:41 thorpej Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1
MKLINT= no

PROG= hpropd

SRCS= 	hpropd.c				\
	hprop-common.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DIST}/openssl/crypto/md4		\
	-I${DIST}/openssl/crypto/md5		\
	-I${DIST}/openssl/crypto/sha		\
	-I${DIST}/openssl/crypto/rc4		\
	-I${DIST}/openssl/crypto/des		\
	-DHAVE_CONFIG_H

LDADD=	-lhdb					\
	-lkrb5					\
	-lkdb					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-lroken					\
	-lcrypt

MAN=	hpropd.8

.include <bsd.prog.mk>
