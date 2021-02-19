#	$NetBSD: Makefile,v 1.2 2021/02/19 16:42:23 christos Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/bind9

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c version.c

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns \
		isccfg	${.CURDIR}/../libisccfg \

version.o version.go version.pico version.po : ${VERSIONFILE}

.include <bsd.lib.mk>
