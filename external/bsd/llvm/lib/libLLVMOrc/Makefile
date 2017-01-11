#	$NetBSD: Makefile,v 1.2 2017/01/11 12:02:30 joerg Exp $

LIB=	LLVMOrc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Orc

SRCS+=	ExecutionUtils.cpp \
	IndirectionUtils.cpp \
	NullResolver.cpp \
	OrcABISupport.cpp \
	OrcCBindings.cpp \
	OrcError.cpp \
	OrcMCJITReplacement.cpp

.include <bsd.lib.mk>
