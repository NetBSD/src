#	$NetBSD: Makefile,v 1.1.14.1 2015/06/04 20:05:44 snj Exp $

LIB=	LLVMObjCARC

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/ObjCARC

SRCS+=	DependencyAnalysis.cpp \
	ObjCARC.cpp \
	ObjCARCAPElim.cpp \
	ObjCARCAliasAnalysis.cpp \
	ObjCARCContract.cpp \
	ObjCARCExpand.cpp \
	ObjCARCOpts.cpp \
	ObjCARCUtil.cpp \
	ProvenanceAnalysis.cpp \
	ProvenanceAnalysisEvaluator.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
