# $NetBSD: Makefile,v 1.1.2.2 2012/11/15 19:49:17 jkunz Exp $

.include <bsd.init.mk>

.PATH:		${DIST}/keygen

.if (${MACHINE} == "evbarm")
PROG_CXX=	sbkeygen
SRCS=		keygen.cpp
.endif

MAN=		sbkeygen.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

