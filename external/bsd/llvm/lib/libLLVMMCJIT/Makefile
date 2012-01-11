#	$NetBSD: Makefile,v 1.4 2012/01/11 23:24:47 joerg Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	Intercept.cpp \
	MCJIT.cpp \
	MCJITMemoryManager.cpp

.include <bsd.lib.mk>
