#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:52 tron Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	milter

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	milter.c milter8.c milter_macros.c

.include <bsd.lib.mk>
