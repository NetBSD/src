#	$NetBSD: Makefile,v 1.1.24.2 2017/04/25 20:54:00 snj Exp $

LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns

DIST=	${IDIST}/lib/isccfg
.include "${DIST}/api"

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c dnsconf.c log.c namedconf.c parser.c version.c 

.include <bsd.lib.mk>
