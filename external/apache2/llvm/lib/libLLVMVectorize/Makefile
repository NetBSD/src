#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:13 joerg Exp $

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
	VPlanPredicator.cpp \
	VPlanSLP.cpp \
	VPlanVerifier.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
