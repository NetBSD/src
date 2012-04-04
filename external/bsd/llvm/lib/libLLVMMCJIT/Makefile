#	$NetBSD: Makefile,v 1.5 2012/04/04 10:42:03 joerg Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	MCJIT.cpp \
	MCJITMemoryManager.cpp

.include <bsd.lib.mk>
