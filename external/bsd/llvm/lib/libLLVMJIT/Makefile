#	$NetBSD: Makefile,v 1.1.4.1 2011/06/23 14:18:30 cherry Exp $

LIB=	LLVMJIT

.include <bsd.init.mk>

CPPFLAGS+=	-DENABLE_X86_JIT

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JIT

SRCS+=	Intercept.cpp \
	JIT.cpp \
	JITDebugRegisterer.cpp \
	JITDwarfEmitter.cpp \
	JITEmitter.cpp \
	JITMemoryManager.cpp \
	OProfileJITEventListener.cpp

.include <bsd.lib.mk>
