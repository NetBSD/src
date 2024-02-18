#	$NetBSD: Makefile,v 1.1 2024/02/18 20:58:01 christos Exp $

LIBISPRIVATE=yes
LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns

DIST=	${IDIST}/lib/isccfg

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c kaspconf.c log.c namedconf.c parser.c version.c dnsconf.c

.include <bsd.lib.mk>
