# $NetBSD: Makefile,v 1.2.2.2 2013/01/16 05:27:57 yamt Exp $

.include <bsd.init.mk>

.PATH:		${DIST}/keygen

.if (${MACHINE} == "evbarm") && (${MACHINE_ARCH} == "arm")
PROG_CXX=	sbkeygen
SRCS=		keygen.cpp
.endif

MAN=		sbkeygen.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

