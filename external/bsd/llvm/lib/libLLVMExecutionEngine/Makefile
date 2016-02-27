#	$NetBSD: Makefile,v 1.6 2016/02/27 21:47:33 joerg Exp $

LIB=	LLVMExecutionEngine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine

SRCS+=	GDBRegistrationListener.cpp \
	ExecutionEngine.cpp \
	ExecutionEngineBindings.cpp \
	SectionMemoryManager.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
