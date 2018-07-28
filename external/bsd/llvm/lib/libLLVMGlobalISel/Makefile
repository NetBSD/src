#	$NetBSD: Makefile,v 1.2.2.1 2018/07/28 04:37:11 pgoyette Exp $

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
