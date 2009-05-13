# $NetBSD: Makefile,v 1.23.10.1 2009/05/13 19:19:53 jym Exp $

WARNS?=	2	# XXX -Wextra

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes	# cryptographic software

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

PROG= kinit

SRCS= kinit.c

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/kuser		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${DESTDIR}/usr/include/krb5		\
	 -DHAVE_CONFIG_H

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkafs -lkrb5 -lhx509 -lheimntlm -ldes -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBDES} \
	${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} \
	${LIBCRYPT} ${LIBUTIL}

.include <bsd.prog.mk>
