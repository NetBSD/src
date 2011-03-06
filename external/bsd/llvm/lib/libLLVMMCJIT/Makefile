#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:48 bouyer Exp $

LIB=	LLVMMCJIT

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/MCJIT

SRCS+=	MCJIT.cpp \
	TargetSelect.cpp

.include <bsd.lib.mk>
