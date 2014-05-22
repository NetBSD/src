#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:44 yamt Exp $

LIB=	MCLinkerCodeGen

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/CodeGen

SRCS=	MCLDTargetMachine.cpp \
	MCLinker.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
