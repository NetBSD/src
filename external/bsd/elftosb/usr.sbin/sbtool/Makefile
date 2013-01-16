# $NetBSD: Makefile,v 1.2.2.2 2013/01/16 05:27:58 yamt Exp $

.include <bsd.init.mk>

.PATH:		${DIST}/sbtool

.if (${MACHINE} == "evbarm") && (${MACHINE_ARCH} == "arm")
PROG_CXX=	sbtool
SRCS=		EncoreBootImageReader.cpp sbtool.cpp
.endif

MAN=		sbtool.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

