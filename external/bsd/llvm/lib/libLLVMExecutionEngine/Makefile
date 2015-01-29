#	$NetBSD: Makefile,v 1.4 2015/01/29 20:41:35 joerg Exp $

LIB=	LLVMExecutionEngine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine

SRCS+=	GDBRegistrationListener.cpp \
	ExecutionEngine.cpp \
	ExecutionEngineBindings.cpp \
	RTDyldMemoryManager.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
