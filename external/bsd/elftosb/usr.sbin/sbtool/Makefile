# $NetBSD: Makefile,v 1.2 2012/11/27 20:00:39 jkunz Exp $

.include <bsd.init.mk>

.PATH:		${DIST}/sbtool

.if (${MACHINE} == "evbarm") && (${MACHINE_ARCH} == "arm")
PROG_CXX=	sbtool
SRCS=		EncoreBootImageReader.cpp sbtool.cpp
.endif

MAN=		sbtool.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

