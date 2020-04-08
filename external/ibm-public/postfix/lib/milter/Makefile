#	$NetBSD: Makefile,v 1.1.50.1 2020/04/08 14:07:00 martin Exp $

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
