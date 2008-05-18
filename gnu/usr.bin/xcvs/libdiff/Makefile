#	$NetBSD: Makefile,v 1.1.18.1 2008/05/18 12:30:09 yamt Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	echo ${DIST}/*.c | sed -e s,${DIST}/,,g

.include <bsd.lib.mk>
