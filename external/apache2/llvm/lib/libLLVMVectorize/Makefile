#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:10 cjep Exp $

LIB=	LLVMVectorize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize

SRCS+=	LoadStoreVectorizer.cpp \
	LoopVectorizationLegality.cpp \
	LoopVectorize.cpp \
	SLPVectorizer.cpp \
	VectorCombine.cpp \
	Vectorize.cpp \
	VPlan.cpp \
	VPlanHCFGBuilder.cpp \
	VPlanPredicator.cpp \
	VPlanSLP.cpp \
	VPlanTransforms.cpp \
	VPlanVerifier.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
