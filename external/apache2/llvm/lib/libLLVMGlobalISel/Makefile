#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:08 cjep Exp $

LIB=	LLVMGlobalISel

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen/GlobalISel

SRCS+=	CallLowering.cpp \
	Combiner.cpp \
	CombinerHelper.cpp \
	CSEInfo.cpp \
	CSEMIRBuilder.cpp \
	GISelChangeObserver.cpp \
	GISelKnownBits.cpp \
	GlobalISel.cpp \
	InlineAsmLowering.cpp \
	InstructionSelect.cpp \
	InstructionSelector.cpp \
	IRTranslator.cpp \
	LegalityPredicates.cpp \
	LegalizeMutations.cpp \
	Legalizer.cpp \
	LegalizerHelper.cpp \
	LegalizerInfo.cpp \
	Localizer.cpp \
	LostDebugLocObserver.cpp \
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
