#	$NetBSD: Makefile,v 1.8 2018/07/17 18:55:29 joerg Exp $

LIB=	LLVMVectorize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize

SRCS+=	LoadStoreVectorizer.cpp \
	LoopVectorizationLegality.cpp \
	LoopVectorize.cpp \
	SLPVectorizer.cpp \
	Vectorize.cpp \
	VPlan.cpp \
	VPlanHCFGBuilder.cpp \
	VPlanHCFGTransforms.cpp \
	VPlanVerifier.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
