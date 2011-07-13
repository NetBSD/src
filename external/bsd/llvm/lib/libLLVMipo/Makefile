#	$NetBSD: Makefile,v 1.3 2011/07/13 23:48:30 joerg Exp $

LIB=	LLVMipo

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/IPO

SRCS+=	ArgumentPromotion.cpp \
	ConstantMerge.cpp \
	DeadArgumentElimination.cpp \
	ExtractGV.cpp \
	FunctionAttrs.cpp \
	GlobalDCE.cpp \
	GlobalOpt.cpp \
	IPConstantPropagation.cpp \
	IPO.cpp \
	InlineAlways.cpp \
	InlineSimple.cpp \
	Inliner.cpp \
	Internalize.cpp \
	LoopExtractor.cpp \
	LowerSetJmp.cpp \
	MergeFunctions.cpp \
	PartialInlining.cpp \
	PruneEH.cpp \
	StripDeadPrototypes.cpp \
	StripSymbols.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
