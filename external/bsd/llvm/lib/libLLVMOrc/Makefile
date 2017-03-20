#	$NetBSD: Makefile,v 1.1.2.1 2017/03/20 06:56:01 pgoyette Exp $

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
