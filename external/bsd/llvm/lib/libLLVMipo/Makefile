#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:05 joerg Exp $

LIB=	LLVMipo

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/IPO

SRCS+=	ArgumentPromotion.cpp \
	ConstantMerge.cpp \
	DeadArgumentElimination.cpp \
	DeadTypeElimination.cpp \
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
	StripSymbols.cpp \
	StructRetPromotion.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
