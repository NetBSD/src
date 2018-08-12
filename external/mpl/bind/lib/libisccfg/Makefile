#	$NetBSD: Makefile,v 1.1 2018/08/12 13:02:43 christos Exp $

LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns

DIST=	${IDIST}/lib/isccfg
.include "${DIST}/api"

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c log.c namedconf.c parser.c version.c dnsconf.c

.include <bsd.lib.mk>
