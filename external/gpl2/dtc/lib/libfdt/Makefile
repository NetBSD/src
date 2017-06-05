#	$NetBSD: Makefile,v 1.1 2017/06/05 18:59:43 christos Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	fdt

.include "${.CURDIR}/../../Makefile.inc"
.include "${.CURDIR}/../../dist/libfdt/Makefile.libfdt"

SRCS=	${LIBFDT_SRCS}

CPPFLAGS+=	-I ${DIST}/libfdt

.PATH:	${DIST}/libfdt

.include <bsd.lib.mk>

