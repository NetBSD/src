#	$NetBSD: Makefile,v 1.3 2018/02/04 03:19:52 christos Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB=		paa

SRCS=		b64.c libpaa.c
CPPFLAGS+=	-I${.CURDIR}/../dist/include -I${.CURDIR}/../dist/src/pgp2ssh

LIBDPLIBS+=	mj	${.CURDIR}/../libmj
LIBDPLIBS+=	netpgp	${.CURDIR}/../lib
LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto
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
