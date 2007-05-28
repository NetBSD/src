# $NetBSD: Makefile,v 1.3 2007/05/28 12:06:27 tls Exp $

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes	# cryptographic software

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1

PROG= kgetcred

SRCS= kgetcred.c

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/kuser		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${DESTDIR}/usr/include/krb5		\
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkrb5 -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
