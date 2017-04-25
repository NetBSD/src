#	$NetBSD: Makefile,v 1.1.16.2 2017/04/25 22:02:05 snj Exp $

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
