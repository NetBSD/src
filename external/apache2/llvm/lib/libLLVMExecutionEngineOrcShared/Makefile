#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:07 cjep Exp $

LIB=	LLVMExecutionEngineOrcShared

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Orc/Shared

SRCS+=	OrcError.cpp \
	RPCError.cpp \
	TargetProcessControlTypes.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
