#	$NetBSD: Makefile,v 1.1.10.1 2014/12/25 17:54:37 msaitoh Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/bind9
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c version.c

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns \
		isccfg	${.CURDIR}/../libisccfg

.include <bsd.lib.mk>
