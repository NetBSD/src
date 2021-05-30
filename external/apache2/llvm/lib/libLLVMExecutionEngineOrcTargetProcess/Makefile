#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:52 joerg Exp $

LIB=	LLVMExecutionEngineOrcTargetProcess

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Orc/TargetProcess

SRCS+=	JITLoaderGDB.cpp \
	RegisterEHFrames.cpp \
	TargetExecutionUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
