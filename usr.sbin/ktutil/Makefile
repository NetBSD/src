# $NetBSD: Makefile,v 1.2 2000/06/17 00:04:38 thorpej Exp $
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
		 remove.c			\
		 srvconvert.c			\
		 srvcreate.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/admin			\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-I${DESTDIR}/usr/include/openssl	\
	-DHAVE_CONFIG_H

LDADD=	-lkadm5srv \
	-lhdb \
	-lkrb5 \
	-lcrypto \
	-lasn1 \
	-lcom_err \
	-lsl \
	-ledit -ltermcap \
	-lroken \
	-lcrypt

MAN = ktutil.8

.include <bsd.prog.mk>
