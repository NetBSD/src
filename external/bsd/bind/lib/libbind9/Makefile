#	$NetBSD: Makefile,v 1.1 2009/04/12 03:46:10 christos Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/bind9
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c version.c

.include <bsd.lib.mk>
