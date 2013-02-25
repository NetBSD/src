#	$NetBSD: Makefile,v 1.5.2.2 2013/02/25 00:26:39 tls Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	MCJIT.cpp \
	SectionMemoryManager.cpp

.include <bsd.lib.mk>
