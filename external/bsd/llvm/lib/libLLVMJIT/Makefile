#	$NetBSD: Makefile,v 1.4 2012/04/04 10:42:03 joerg Exp $

LIB=	LLVMJIT

.include <bsd.init.mk>

CPPFLAGS+=	-DENABLE_X86_JIT

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JIT

SRCS+=	JIT.cpp \
	JITDwarfEmitter.cpp \
	JITEmitter.cpp \
	JITMemoryManager.cpp

.include <bsd.lib.mk>
