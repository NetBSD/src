#	$NetBSD: Makefile,v 1.3.4.2 2017/07/18 17:17:53 snj Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	fdt

.include "${.CURDIR}/../../Makefile.inc"
.include "${LIBFDTDIST}/Makefile.libfdt"

SRCS=	${LIBFDT_SRCS}

CPPFLAGS+=	-I ${LIBFDTDIST}

.PATH:	${LIBFDTDIST}

.include <bsd.lib.mk>

