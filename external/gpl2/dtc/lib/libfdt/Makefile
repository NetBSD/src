#	$NetBSD: Makefile,v 1.3 2017/06/08 18:57:21 skrll Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	fdt

.include "${.CURDIR}/../../Makefile.inc"
.include "${LIBFDTDIST}/Makefile.libfdt"

SRCS=	${LIBFDT_SRCS}

CPPFLAGS+=	-I ${LIBFDTDIST}

.PATH:	${LIBFDTDIST}

.include <bsd.lib.mk>

