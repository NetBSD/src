#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:07 cjep Exp $

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
