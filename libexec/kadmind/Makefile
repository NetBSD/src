# $NetBSD: Makefile,v 1.2.4.2 2000/06/22 15:58:20 minoura Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kadmin

WARNS?=  1
MKLINT= no

PROG= kadmind

SRCS= 	kadmind.c				\
	server.c				\
	version4.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kadmin		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DIST}/heimdal/lib/asn1		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DIST}/openssl/crypto/md4		\
	-I${DIST}/openssl/crypto/md5		\
	-I${DIST}/openssl/crypto/sha		\
	-I${DIST}/openssl/crypto/rc4		\
	-I${DIST}/openssl/crypto/des		\
	-DHAVE_CONFIG_H

LDADD=	-lkadm5srv				\
	-lhdb					\
	-lkrb5					\
	-lkadm					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-lroken					\
	-lcrypt

NOMAN=

.include <bsd.prog.mk>
