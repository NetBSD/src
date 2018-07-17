#	$NetBSD: Makefile,v 1.3 2018/07/17 18:55:28 joerg Exp $

LIB=	LLVMGlobalISel

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/GlobalISel

SRCS+=	CallLowering.cpp \
	Combiner.cpp \
	CombinerHelper.cpp \
	GlobalISel.cpp \
	InstructionSelect.cpp \
	InstructionSelector.cpp \
	IRTranslator.cpp \
	LegalityPredicates.cpp \
	LegalizeMutations.cpp \
	Legalizer.cpp \
	LegalizerHelper.cpp \
	LegalizerInfo.cpp \
	Localizer.cpp \
	MachineIRBuilder.cpp \
	RegBankSelect.cpp \
	RegisterBank.cpp \
	RegisterBankInfo.cpp \
	Utils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
