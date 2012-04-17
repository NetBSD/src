#	$NetBSD: Makefile,v 1.1.8.1 2012/04/17 00:04:15 yamt Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	(cd ${DIST} && echo *.c)

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.lib.mk>
