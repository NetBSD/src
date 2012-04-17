#	$NetBSD: Makefile,v 1.2.2.1 2012/04/17 00:03:31 yamt Exp $

LIB=	LLVMJIT

.include <bsd.init.mk>

CPPFLAGS+=	-DENABLE_X86_JIT

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JIT

SRCS+=	JIT.cpp \
	JITDwarfEmitter.cpp \
	JITEmitter.cpp \
	JITMemoryManager.cpp

.include <bsd.lib.mk>
