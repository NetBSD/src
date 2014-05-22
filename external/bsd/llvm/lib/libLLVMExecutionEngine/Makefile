#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 16:14:41 yamt Exp $

LIB=	LLVMExecutionEngine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine

SRCS+=	ExecutionEngine.cpp \
	ExecutionEngineBindings.cpp \
	RTDyldMemoryManager.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
