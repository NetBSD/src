#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:00 joerg Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	MCJIT.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
