# $NetBSD: Makefile,v 1.19.10.1 2009/05/13 19:19:53 jym Exp $

USE_FORT?= yes	# cryptographic software

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

PROG= kdestroy

SRCS= kdestroy.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kuser			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+= -lkafs -lkrb5 -lhx509 -lcrypto -lasn1 \
	-lcom_err -L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
