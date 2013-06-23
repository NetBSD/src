#	$NetBSD: Makefile,v 1.1.6.2 2013/06/23 06:28:24 tls Exp $

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
