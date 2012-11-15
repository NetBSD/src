# $NetBSD: Makefile,v 1.1 2012/11/15 19:49:17 jkunz Exp $

.include <bsd.init.mk>

.PATH:		${DIST}/sbtool

.if (${MACHINE} == "evbarm")
PROG_CXX=	sbtool
SRCS=		EncoreBootImageReader.cpp sbtool.cpp
.endif

MAN=		sbtool.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

