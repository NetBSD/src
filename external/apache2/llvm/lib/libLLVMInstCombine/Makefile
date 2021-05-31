#	$NetBSD: Makefile,v 1.2.2.1 2021/05/31 22:10:08 cjep Exp $

LIB=	LLVMInstCombine

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/InstCombine

SRCS+=	InstCombineAddSub.cpp \
	InstCombineAndOrXor.cpp \
	InstCombineAtomicRMW.cpp \
	InstCombineCalls.cpp \
	InstCombineCasts.cpp \
	InstCombineCompares.cpp \
	InstCombineLoadStoreAlloca.cpp \
	InstCombineMulDivRem.cpp \
	InstCombineNegator.cpp \
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

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}
