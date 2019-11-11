#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:07 joerg Exp $

LIB=	LLVMInterpreter

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Interpreter

SRCS+=	Execution.cpp \
	ExternalFunctions.cpp \
	Interpreter.cpp

.include <bsd.lib.mk>
