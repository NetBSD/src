# $NetBSD: Makefile,v 1.9 2001/02/11 18:02:04 assar Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/admin

WARNS?=  1
MKLINT= no

PROG= ktutil

SRCS=		 add.c				\
		 change.c			\
		 copy.c				\
		 get.c				\
		 ktutil.c			\
		 list.c				\
		 purge.c			\
		 remove.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/admin			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include		\
	-I${DESTDIR}/usr/include/openssl	\
	-DHAVE_CONFIG_H

VERS!=	cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=	-lkadm5clnt \
	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-lsl \
	-ledit -ltermcap \
	-L${VERS} -lvers \
	-lroken \
	-lcrypt

MAN = ktutil.8

.include <bsd.prog.mk>
