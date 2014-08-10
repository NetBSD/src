#	$NetBSD: Makefile,v 1.1.2.2 2014/08/10 07:09:10 tls Exp $

LIB=	lldbPluginInstructionARM64

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Instruction/ARM64

SRCS+=	EmulateInstructionARM64.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
