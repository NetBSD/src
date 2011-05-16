#	$NetBSD: Makefile,v 1.2 2011/05/16 19:27:48 joerg Exp $

LIB=	LLVMExecutionEngine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine

SRCS+=	ExecutionEngine.cpp \
	ExecutionEngineBindings.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
