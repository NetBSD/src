#	$NetBSD: Makefile,v 1.1.10.1 2013/06/03 02:45:25 riz Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	(cd ${DIST} && echo *.c)

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.lib.mk>
