#	$NetBSD: Makefile,v 1.3 2012/01/11 23:24:47 joerg Exp $

LIB=	LLVMJIT

.include <bsd.init.mk>

CPPFLAGS+=	-DENABLE_X86_JIT

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JIT

SRCS+=	Intercept.cpp \
	JIT.cpp \
	JITDwarfEmitter.cpp \
	JITEmitter.cpp \
	JITMemoryManager.cpp \
	OProfileJITEventListener.cpp

.include <bsd.lib.mk>
