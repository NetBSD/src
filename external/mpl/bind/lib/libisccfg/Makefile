#	$NetBSD: Makefile,v 1.4.2.1 2025/08/02 05:54:21 perseant Exp $

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
