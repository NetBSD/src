#	$NetBSD: Makefile,v 1.4.2.1 2013/06/23 06:28:23 tls Exp $

LIB=	LLVMJIT

.include <bsd.init.mk>

CPPFLAGS+=	-DENABLE_X86_JIT

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/JIT

SRCS+=	JIT.cpp \
	JITEmitter.cpp \
	JITMemoryManager.cpp

.include <bsd.lib.mk>
