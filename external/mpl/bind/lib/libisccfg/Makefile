#	$NetBSD: Makefile,v 1.3 2021/02/19 16:42:24 christos Exp $

LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns

DIST=	${IDIST}/lib/isccfg

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c kaspconf.c log.c namedconf.c parser.c version.c dnsconf.c

.include <bsd.lib.mk>
