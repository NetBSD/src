# $NetBSD: Makefile,v 1.2.2.1 2001/04/05 23:25:23 he Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kadmin

WARNS?=  1
MKLINT= no

PROG= kadmind

SRCS= 	kadmind.c				\
	server.c				\
	version4.c				\
	kadm_conn.c

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

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-lkadm5srv				\
	-lhdb					\
	-lkrb5					\
	-lkadm					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-L${VERS} -lvers			\
	-lroken					\
	-lcrypt					\
	-lutil

MAN=	kadmind.8

.include <bsd.prog.mk>
