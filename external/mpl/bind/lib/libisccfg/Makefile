#	$NetBSD: Makefile,v 1.1.6.1 2024/02/29 12:36:04 martin Exp $

NOLINT=yes	# XXX
LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc \
		dns	${.CURDIR}/../libdns

DIST=	${IDIST}/lib/isccfg

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c duration.c kaspconf.c log.c namedconf.c parser.c dnsconf.c

.include <bsd.lib.mk>
