#	$NetBSD: Makefile,v 1.2 2011/06/01 14:13:42 joerg Exp $

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
