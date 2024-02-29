#	$NetBSD: Makefile,v 1.1.6.1 2024/02/29 12:36:03 martin Exp $

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
