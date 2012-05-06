#	$NetBSD: Makefile,v 1.1.2.1 2012/05/06 18:07:02 agc Exp $

.include <bsd.own.mk>

USE_FORT?= yes

LIB=		paa

SRCS=		b64.c libpaa.c
CPPFLAGS+=	-I${EXTDIST}/include -I${EXTDIST}/src/pgp2ssh

LIBDPLIBS+=	mj	${.CURDIR}/../mj
LIBDPLIBS+=	netpgp	${.CURDIR}/../netpgp
LIBDPLIBS+=	netpgpverify	${.CURDIR}/../verify
LIBDPLIBS+=	netpgpbn	${.CURDIR}/../bn
LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2	${NETBSDSRCDIR}/lib/libbz2

MAN=		libpaa.3
WARNS=		4

EXTDIST=	${.CURDIR}/../../dist
.PATH: ${EXTDIST}/src/pgp2ssh ${EXTDIST}/src/libpaa

INCS+= libpaa.h
INCSDIR=/usr/include/netpgp

.include <bsd.lib.mk>

SUBDIR+=	client server

.include <bsd.subdir.mk>

t:
	server/paaserver -r authentication@bigco.com
