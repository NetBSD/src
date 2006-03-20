# $NetBSD: Makefile,v 1.15 2006/03/20 04:03:23 christos Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kuser

WARNS?=  1

PROG= klist

SRCS= klist.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kuser			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkafs -lkrb5 ${KRB4LDADD} -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKAFS} ${LIBKRB5} ${KRB4DPADD} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
