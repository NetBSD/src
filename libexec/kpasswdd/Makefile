# $NetBSD: Makefile,v 1.4 2000/08/03 22:47:41 assar Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/kpasswd

WARNS?=  1
MKLINT= no

PROG= kpasswdd

SRCS= 	kpasswdd.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kpasswd		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DIST}/openssl/crypto/md4		\
	-I${DIST}/openssl/crypto/md5		\
	-I${DIST}/openssl/crypto/sha		\
	-I${DIST}/openssl/crypto/rc4		\
	-I${DIST}/openssl/crypto/des		\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && make print-objdir

LDADD=	-lkadm5srv \
	-lhdb \
	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-L${VERS} -lvers \
	-lroken \
	-lcrypt

MAN = kpasswdd.8

.include <bsd.prog.mk>
