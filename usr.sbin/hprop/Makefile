# $NetBSD: Makefile,v 1.16 2008/08/29 00:02:25 gmcgarry Exp $

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes		# network client

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1

PROG= hprop

SRCS+= 	hprop.c					\
	v4_dump.c				\
	mit_dump.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${DIST}/heimdal/lib/hdb		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS.mit_dump.c+=	-Wno-pointer-sign
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lhdb -lkrb5 -lhx509 -lcrypto -lasn1 \
	-lcom_err -L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

MAN=	hprop.8

.include <bsd.prog.mk>
