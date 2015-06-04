#	$NetBSD: Makefile,v 1.3.6.1 2015/06/04 20:05:44 snj Exp $

LIB=	LLVMExecutionEngine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine

SRCS+=	GDBRegistrationListener.cpp \
	ExecutionEngine.cpp \
	ExecutionEngineBindings.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
