#	$NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:39 tls Exp $

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
	ProvenanceAnalysis.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
