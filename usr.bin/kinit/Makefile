# $NetBSD: Makefile,v 1.22 2008/03/22 08:37:27 mlelstv Exp $

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes	# cryptographic software

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1

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

LDADD+=	-lkafs -lkrb5 -lheimntlm -ldes -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBHEIMNTLM} ${LIBDES} ${LIBCRYPTO} \
	${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
