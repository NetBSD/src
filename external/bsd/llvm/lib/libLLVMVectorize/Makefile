#	$NetBSD: Makefile,v 1.7.2.1 2018/07/28 04:37:12 pgoyette Exp $

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
