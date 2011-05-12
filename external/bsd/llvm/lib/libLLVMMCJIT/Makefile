#	$NetBSD: Makefile,v 1.2 2011/05/12 14:56:23 joerg Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	Intercept.cpp \
	MCJIT.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
