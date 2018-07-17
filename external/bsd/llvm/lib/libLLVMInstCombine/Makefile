#	$NetBSD: Makefile,v 1.2 2018/07/17 18:55:28 joerg Exp $

LIB=	LLVMInstCombine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/InstCombine

TABLEGEN_SRC=		InstCombineTables.td
TABLEGEN_OUTPUT= \
	InstCombineTables.inc|-gen-searchable-tables

SRCS+=	InstructionCombining.cpp \
	InstCombineAddSub.cpp \
	InstCombineAndOrXor.cpp \
	InstCombineCalls.cpp \
	InstCombineCasts.cpp \
	InstCombineCompares.cpp \
	InstCombineLoadStoreAlloca.cpp \
	InstCombineMulDivRem.cpp \
	InstCombinePHI.cpp \
	InstCombineSelect.cpp \
	InstCombineShifts.cpp \
	InstCombineSimplifyDemanded.cpp \
	InstCombineVectorOps.cpp

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
