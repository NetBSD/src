#	$NetBSD: Makefile,v 1.2.6.1 2024/02/25 15:47:57 martin Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/bind9

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns \
		isccfg	${.CURDIR}/../libisccfg \

.include <bsd.lib.mk>
