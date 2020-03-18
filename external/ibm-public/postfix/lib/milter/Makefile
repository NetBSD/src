#	$NetBSD: Makefile,v 1.2 2020/03/18 19:05:22 christos Exp $

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
