#	$NetBSD: Makefile,v 1.2.10.1 2013/06/23 06:28:23 tls Exp $

LIB=	LLVMExecutionEngine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine

SRCS+=	ExecutionEngine.cpp \
	ExecutionEngineBindings.cpp \
	RTDyldMemoryManager.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
