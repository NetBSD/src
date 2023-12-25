#	$NetBSD: Makefile,v 1.1.52.1 2023/12/25 12:55:37 martin Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	milter

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS+= \
milter.c \
milter8.c \
milter_macros.c

.include <bsd.lib.mk>
