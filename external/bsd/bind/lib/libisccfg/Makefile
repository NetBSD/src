#	$NetBSD: Makefile,v 1.1 2009/04/12 03:46:10 christos Exp $

LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isccfg
.include "${DIST}/api"

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c log.c namedconf.c parser.c version.c 

.include <bsd.lib.mk>
