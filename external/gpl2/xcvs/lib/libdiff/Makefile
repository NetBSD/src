#	$NetBSD: Makefile,v 1.2 2012/04/04 10:59:44 joerg Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	(cd ${DIST} && echo *.c)

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.lib.mk>
