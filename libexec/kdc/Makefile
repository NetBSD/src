# $NetBSD: Makefile,v 1.4 2000/08/03 22:47:40 assar Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1
MKLINT= no

PROG= kdc

SRCS= 	524.c		\
	config.c	\
	connect.c	\
	kaserver.c	\
	kdc_locl.h	\
	kerberos4.c	\
	kerberos4.h	\
	kerberos5.c	\
	log.c		\
	main.c		\
	misc.c		\

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

VERS!=	cd ${.CURDIR}/../../lib/libvers && make print-objdir

LDADD=	-lhdb					\
	-lkrb5					\
	-lkrb					\
	-lcrypto				\
	-lasn1					\
	-lcom_err				\
	-L${VERS} -lvers			\
	-lroken					\
	-lcrypt

MAN=	kdc.8

.include <bsd.prog.mk>
