#	$NetBSD: Makefile,v 1.3.2.3 2014/05/22 16:14:42 yamt Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	MCJIT.cpp \
	SectionMemoryManager.cpp

.include <bsd.lib.mk>
