#	$NetBSD: Makefile,v 1.2.2.2 2014/05/22 16:14:42 yamt Exp $

LIB=	LLVMJIT

.include <bsd.init.mk>

CPPFLAGS+=	-DENABLE_X86_JIT

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JIT

SRCS+=	JIT.cpp \
	JITEmitter.cpp \
	JITMemoryManager.cpp

.include <bsd.lib.mk>
