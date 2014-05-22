#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:49 yamt Exp $

LIB=	lldbPluginInstructionARM

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Plugins/Instruction/ARM

SRCS+=	EmulateInstructionARM.cpp \
	EmulationStateARM.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
