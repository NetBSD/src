#	$NetBSD: Makefile,v 1.4 2025/07/14 10:25:50 kre Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB=		paa

SRCS=		b64.c libpaa.c
CPPFLAGS+=	-I${.CURDIR}/../dist/include -I${.CURDIR}/../dist/src/pgp2ssh

LIBDPLIBS+=	mj	${.CURDIR}/../libmj
LIBDPLIBS+=	netpgp	${.CURDIR}/../lib
LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto
LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2	${NETBSDSRCDIR}/lib/libbz2

MAN=		libpaa.3

.PATH: ${.CURDIR}/../dist/src/pgp2ssh ${.CURDIR}/../dist/src/libpaa

INCS+= libpaa.h
INCSDIR=/usr/include

.include <bsd.lib.mk>

SUBDIR+=	client server

.include <bsd.subdir.mk>

t:
	server/paaserver -r authentication@bigco.com
