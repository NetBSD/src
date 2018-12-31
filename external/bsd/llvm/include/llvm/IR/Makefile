#	$NetBSD: Makefile,v 1.1 2018/12/31 19:33:04 maya Exp $

.include <bsd.init.mk>

.PATH:	${LLVM_SRCDIR}/include/llvm/IR \
	${LLVM_SRCDIR}/include/llvm/CodeGen # For RuntimeLibcalls.def

INCS=	Argument.h \
	AssemblyAnnotationWriter.h \
	Attributes.h \
	AutoUpgrade.h \
	BasicBlock.h \
	CFG.h \
	CMakeLists.txt \
	CallSite.h \
	CallingConv.h \
	Comdat.h \
	Constant.h \
	ConstantFolder.h \
	ConstantRange.h \
	Constants.h \
	DIBuilder.h \
	DataLayout.h \
	DebugInfo.h \
	DebugInfoFlags.def \
	DebugInfoMetadata.h \
	DebugLoc.h \
	DerivedTypes.h \
	DerivedUser.h \
	DiagnosticHandler.h \
	DiagnosticInfo.h \
	DiagnosticPrinter.h \
	DomTreeUpdater.h \
	Dominators.h \
	Function.h \
	GVMaterializer.h \
	GetElementPtrTypeIterator.h \
	GlobalAlias.h \
	GlobalIFunc.h \
	GlobalIndirectSymbol.h \
	GlobalObject.h \
	GlobalValue.h \
	GlobalVariable.h \
	IRBuilder.h \
	IRPrintingPasses.h \
	InlineAsm.h \
	InstIterator.h \
	InstVisitor.h \
	InstrTypes.h \
	Instruction.def \
	Instruction.h \
	Instructions.h \
	IntrinsicInst.h \
	Intrinsics.h \
	LLVMContext.h \
	LegacyPassManager.h \
	LegacyPassManagers.h \
	LegacyPassNameParser.h \
	MDBuilder.h \
	Mangler.h \
	Metadata.def \
	Metadata.h \
	Module.h \
	ModuleSlotTracker.h \
	ModuleSummaryIndex.h \
	ModuleSummaryIndexYAML.h \
	NoFolder.h \
	OperandTraits.h \
	Operator.h \
	OptBisect.h \
	PassManager.h \
	PassManagerInternal.h \
	PatternMatch.h \
	PredIteratorCache.h \
	ProfileSummary.h \
	RuntimeLibcalls.def \
	SafepointIRVerifier.h \
	Statepoint.h \
	SymbolTableListTraits.h \
	TrackingMDRef.h \
	Type.h \
	TypeBuilder.h \
	TypeFinder.h \
	Use.h \
	UseListOrder.h \
	User.h \
	Value.def \
	Value.h \
	ValueHandle.h \
	ValueMap.h \
	ValueSymbolTable.h \
	Verifier.h

#	Not yet, need tablegen at tool time
#	Attributes.inc
#	IntrinsicEnums.inc
#	IntrinsicImpl.inc

INCSDIR=	/usr/include/llvm/IR

.include <bsd.prog.mk>
