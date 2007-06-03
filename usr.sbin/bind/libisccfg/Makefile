#	$NetBSD: Makefile,v 1.1.1.1.12.1 2007/06/03 17:26:14 wrstuden Exp $

LIB=isccfg

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isccfg
.include "${DIST}/api"

.PATH.c:	${DIST}/unix ${DIST}

SRCS=	aclconf.c log.c namedconf.c parser.c version.c 

.include <bsd.lib.mk>
