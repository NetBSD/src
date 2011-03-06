#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:48 bouyer Exp $

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
