#	$NetBSD: Makefile,v 1.5 2025/01/26 16:25:53 christos Exp $

LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns

DIST=	${IDIST}/lib/isccfg

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c check.c duration.c kaspconf.c log.c namedconf.c parser.c \
	dnsconf.c

.include <bsd.lib.mk>
