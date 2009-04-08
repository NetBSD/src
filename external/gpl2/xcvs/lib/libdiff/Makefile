#	$NetBSD: Makefile,v 1.1 2009/04/08 16:27:52 christos Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	(cd ${DIST} && echo *.c)

.include <bsd.lib.mk>
