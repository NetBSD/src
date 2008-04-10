# $NetBSD: Makefile,v 1.17 2008/04/10 15:05:24 tsutsui Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1

PROG= string2key
MAN=  string2key.8

SRCS= string2key.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkrb5 -lhx509 -lcrypto -lasn1 -lcom_err -L${VERS} \
	-lvers -lroken -lcrypt -lutil
DPADD+=	${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <bsd.prog.mk>
