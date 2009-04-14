# $NetBSD: Makefile,v 1.5 2009/04/14 22:15:21 lukem Exp $

WARNS?=	2	# XXX -Wextra

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes	# cryptographic software

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

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

LDADD+=	-lkrb5 -lhx509 -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <bsd.prog.mk>
