# $NetBSD: Makefile,v 1.31 2013/05/28 17:37:34 joerg Exp $

.include <bsd.own.mk>

SUBDIR=	\
	libLLVMAnalysis \
	libLLVMAsmParser \
	libLLVMAsmPrinter \
	libLLVMBitReader \
	libLLVMBitWriter \
	libLLVMCodeGen \
	libLLVMIR \
	libLLVMIRReader \
	libLLVMInstCombine \
	libLLVMInstrumentation \
	libLLVMipa \
	libLLVMipo \
	libLLVMLinker \
	libLLVMMC \
	libLLVMMCDisassembler \
	libLLVMMCParser \
	libLLVMObjCARC \
	libLLVMObject \
	libLLVMScalarOpts \
	libLLVMSelectionDAG \
	libLLVMSupport \
	libLLVMTableGen \
	libLLVMTarget \
	libLLVMTransformsUtils \
	libLLVMVectorize \
	libLLVMAArch64CodeGen \
	libLLVMARMCodeGen \
	libLLVMMipsCodeGen \
	libLLVMPowerPCCodeGen \
	libLLVMSparcCodeGen \
	libLLVMX86CodeGen

SUBDIR+= .WAIT

SUBDIR+= \
	libLLVMAArch64AsmParser \
	libLLVMAArch64AsmPrinter \
	libLLVMAArch64Disassembler \
	libLLVMAArch64MCTargetDesc \
	libLLVMAArch64TargetInfo \
	libLLVMAArch64Utils

SUBDIR+= \
	libLLVMARMAsmParser \
	libLLVMARMAsmPrinter \
	libLLVMARMDisassembler \
	libLLVMARMMCTargetDesc \
	libLLVMARMTargetInfo

SUBDIR+= \
	libLLVMMipsAsmParser \
	libLLVMMipsAsmPrinter \
	libLLVMMipsDisassembler \
	libLLVMMipsMCTargetDesc \
	libLLVMMipsTargetInfo

SUBDIR+= \
	libLLVMPowerPCAsmParser \
	libLLVMPowerPCAsmPrinter \
	libLLVMPowerPCMCTargetDesc \
	libLLVMPowerPCTargetInfo

SUBDIR+= \
	libLLVMSparcMCTargetDesc \
	libLLVMSparcTargetInfo

SUBDIR+= \
	libLLVMX86AsmParser \
	libLLVMX86AsmPrinter \
	libLLVMX86Disassembler \
	libLLVMX86MCTargetDesc \
	libLLVMX86TargetInfo \
	libLLVMX86Utils

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libLLVMArchive \
	libLLVMDebugInfo \
	libLLVMCppBackend \
	libLLVMCppBackendInfo \
	libLLVMExecutionEngine \
	libLLVMHello \
	libLLVMInterpreter \
	libLLVMJIT \
	libLLVMMCJIT \
	libLLVMOption \
	libLLVMRuntimeDyld
.endif

SUBDIR+= \
	libclangAnalysis \
	libclangARCMigrate \
	libclangAST \
	libclangBasic \
	libclangCodeGen \
	libclangDriver \
	libclangEdit \
	libclangFrontend \
	libclangFrontendTool \
	libclangLex \
	libclangParse \
	libclangRewriteFrontend \
	libclangRewriteCore \
	libclangSema \
	libclangSerialization \
	libclangStaticAnalyzerCheckers \
	libclangStaticAnalyzerCore \
	.WAIT \
	libclangStaticAnalyzerFrontend

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libclangASTMatchers \
	libclangASTMatchersDynamic \
	libclangFormat \
	libclangTooling
.endif

.if ${MKMCLINKER} != "no"
SUBDIR+= \
	libMCLinkerADT \
	libMCLinkerCodeGen \
	libMCLinkerCore \
	libMCLinkerFragment \
	libMCLinkerLD \
	libMCLinkerMC \
	libMCLinkerObject \
	libMCLinkerSupport \
	libMCLinkerTarget

SUBDIR+= \
	libMCLinkerARMCodeGen libMCLinkerARMTargetInfo
SUBDIR+= \
	libMCLinkerMipsCodeGen libMCLinkerMipsTargetInfo
SUBDIR+= \
	libMCLinkerX86CodeGen libMCLinkerX86TargetInfo

.endif

.include <bsd.subdir.mk>
