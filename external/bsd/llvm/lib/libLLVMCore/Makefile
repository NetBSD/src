#	$NetBSD: Makefile,v 1.6.2.2 2012/10/30 18:55:50 yamt Exp $

LIB=	LLVMCore

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/VMCore

SRCS+=	AsmWriter.cpp \
	Attributes.cpp \
	AutoUpgrade.cpp \
	BasicBlock.cpp \
	ConstantFold.cpp \
	Constants.cpp \
	Core.cpp \
	DIBuilder.cpp \
	DataLayout.cpp \
	DebugInfo.cpp \
	DebugLoc.cpp \
	Dominators.cpp \
	Function.cpp \
	GCOV.cpp \
	GVMaterializer.cpp \
	Globals.cpp \
	IRBuilder.cpp \
	InlineAsm.cpp \
	Instruction.cpp \
	Instructions.cpp \
	IntrinsicInst.cpp \
	LLVMContext.cpp \
	LLVMContextImpl.cpp \
	LeakDetector.cpp \
	Metadata.cpp \
	Module.cpp \
	Pass.cpp \
	PassManager.cpp \
	PassRegistry.cpp \
	PrintModulePass.cpp \
	TargetTransformInfo.cpp \
	Type.cpp \
	TypeFinder.cpp \
	Use.cpp \
	User.cpp \
	Value.cpp \
	ValueSymbolTable.cpp \
	ValueTypes.cpp \
	Verifier.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
