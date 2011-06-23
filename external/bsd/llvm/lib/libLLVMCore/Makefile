#	$NetBSD: Makefile,v 1.3.2.1 2011/06/23 14:18:30 cherry Exp $

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
	DebugInfoProbe.cpp \
	DebugLoc.cpp \
	Dominators.cpp \
	Function.cpp \
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
	Type.cpp \
	TypeSymbolTable.cpp \
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
