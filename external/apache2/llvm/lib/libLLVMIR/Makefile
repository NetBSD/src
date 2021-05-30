#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:52 joerg Exp $

LIB=	LLVMIR

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/IR

.include "${.PARSEDIR}/../../tablegen.mk"

SRCS+=	AbstractCallSite.cpp \
	AsmWriter.cpp \
	Assumptions.cpp \
	Attributes.cpp \
	AutoUpgrade.cpp \
	BasicBlock.cpp \
	BuiltinGCs.cpp \
	Comdat.cpp \
	ConstantFold.cpp \
	ConstantRange.cpp \
	Constants.cpp \
	Core.cpp \
	DataLayout.cpp \
	DebugInfo.cpp \
	DebugInfoMetadata.cpp \
	DebugLoc.cpp \
	DiagnosticHandler.cpp \
	DiagnosticInfo.cpp \
	DiagnosticPrinter.cpp \
	DIBuilder.cpp \
	Dominators.cpp \
	FPEnv.cpp \
	Function.cpp \
	GCStrategy.cpp \
	Globals.cpp \
	GVMaterializer.cpp \
	InlineAsm.cpp \
	Instruction.cpp \
	Instructions.cpp \
	IntrinsicInst.cpp \
	IRBuilder.cpp \
	IRPrintingPasses.cpp \
	LegacyPassManager.cpp \
	LLVMContext.cpp \
	LLVMContextImpl.cpp \
	LLVMRemarkStreamer.cpp \
	Mangler.cpp \
	MDBuilder.cpp \
	Metadata.cpp \
	Module.cpp \
	ModuleSummaryIndex.cpp \
	Operator.cpp \
	OptBisect.cpp \
	Pass.cpp \
	PassInstrumentation.cpp \
	PassManager.cpp \
	PassRegistry.cpp \
	PassTimingInfo.cpp \
	PrintPasses.cpp \
	ProfileSummary.cpp \
	PseudoProbe.cpp \
	ReplaceConstant.cpp \
	SafepointIRVerifier.cpp \
	Statepoint.cpp \
	StructuralHash.cpp \
	Type.cpp \
	TypeFinder.cpp \
	Use.cpp \
	User.cpp \
	Value.cpp \
	ValueSymbolTable.cpp \
	Verifier.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
