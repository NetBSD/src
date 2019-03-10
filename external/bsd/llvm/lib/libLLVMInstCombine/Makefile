#	$NetBSD: Makefile,v 1.3 2019/03/10 12:14:06 mrg Exp $

LIB=	LLVMInstCombine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/InstCombine

TABLEGEN_SRC=		InstCombineTables.td
TABLEGEN_OUTPUT= \
	InstCombineTables.inc|-gen-searchable-tables

.include "${.PARSEDIR}/../../tablegen.mk"

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

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
