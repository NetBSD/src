#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:51:49 jym Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/bind9
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c version.c

.include <bsd.lib.mk>
