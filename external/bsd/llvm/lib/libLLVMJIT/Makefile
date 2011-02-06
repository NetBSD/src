#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:58 joerg Exp $

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
	OProfileJITEventListener.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
