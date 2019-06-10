#	$NetBSD: Makefile,v 1.6.16.1 2019/06/10 21:50:52 christos Exp $

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
