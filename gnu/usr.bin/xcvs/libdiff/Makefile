#	$NetBSD: Makefile,v 1.3 2008/10/25 22:10:39 apb Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	(cd ${DIST} && echo *.c)

.include <bsd.lib.mk>
