#	$NetBSD: Makefile,v 1.2.2.1 2014/08/10 07:06:45 tls Exp $

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
