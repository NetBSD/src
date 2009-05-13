#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:53:22 jym Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	(cd ${DIST} && echo *.c)

.include <bsd.lib.mk>
