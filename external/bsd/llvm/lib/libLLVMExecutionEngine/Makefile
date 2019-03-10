#	$NetBSD: Makefile,v 1.7 2019/03/10 12:14:06 mrg Exp $

LIB=	LLVMExecutionEngine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine

SRCS+=	GDBRegistrationListener.cpp \
	ExecutionEngine.cpp \
	ExecutionEngineBindings.cpp \
	SectionMemoryManager.cpp \
	TargetSelect.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
