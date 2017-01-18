#	$NetBSD: Makefile,v 1.2.10.1 2017/01/18 08:46:19 skrll Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/bind9
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c version.c

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns \
		isccfg	${.CURDIR}/../libisccfg \

version.o version.go version.pico version.po : ${VERSIONFILE}

.include <bsd.lib.mk>
