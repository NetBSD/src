#	$NetBSD: Makefile,v 1.1.1.1 2004/05/18 00:06:58 christos Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/bind9
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c version.c

.include <bsd.lib.mk>
