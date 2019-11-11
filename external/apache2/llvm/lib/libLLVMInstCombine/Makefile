#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:07 joerg Exp $

LIB=	LLVMInstCombine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/InstCombine

TABLEGEN_SRC=		InstCombineTables.td
TABLEGEN_OUTPUT= \
	InstCombineTables.inc|-gen-searchable-tables

.include "${.PARSEDIR}/../../tablegen.mk"

SRCS+=	InstCombineAddSub.cpp \
	InstCombineAndOrXor.cpp \
	InstCombineAtomicRMW.cpp \
	InstCombineCalls.cpp \
	InstCombineCasts.cpp \
	InstCombineCompares.cpp \
	InstCombineLoadStoreAlloca.cpp \
	InstCombineMulDivRem.cpp \
	InstCombinePHI.cpp \
	InstCombineSelect.cpp \
	InstCombineShifts.cpp \
	InstCombineSimplifyDemanded.cpp \
	InstCombineVectorOps.cpp \
	InstructionCombining.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
