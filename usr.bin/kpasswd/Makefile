# $NetBSD: Makefile,v 1.12 2002/10/23 01:25:37 itojun Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kpasswd

WARNS?=  1

PROG= kpasswd

SRCS= kpasswd.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kpasswd		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkrb5 -lcrypto -lasn1 -lcom_err -L${VERS} -lvers \
	-lroken -lcrypt
DPADD+=	${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a \
	${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
