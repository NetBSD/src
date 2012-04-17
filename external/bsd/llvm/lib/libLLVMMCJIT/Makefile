#	$NetBSD: Makefile,v 1.3.2.1 2012/04/17 00:03:31 yamt Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	MCJIT.cpp \
	MCJITMemoryManager.cpp

.include <bsd.lib.mk>
