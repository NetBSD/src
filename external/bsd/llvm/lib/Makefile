# $NetBSD: Makefile,v 1.23 2012/06/17 15:12:13 joerg Exp $

.include <bsd.own.mk>

SUBDIR=	\
	libLLVMAnalysis \
	libLLVMAsmParser \
	libLLVMAsmPrinter \
	libLLVMBitReader \
	libLLVMBitWriter \
	libLLVMCodeGen \
	libLLVMCore \
	libLLVMInstCombine \
	libLLVMInstrumentation \
	libLLVMipa \
	libLLVMipo \
	libLLVMLinker \
	libLLVMMC \
	libLLVMMCDisassembler \
	libLLVMMCParser \
	libLLVMScalarOpts \
	libLLVMSelectionDAG \
	libLLVMSupport \
	libLLVMTableGen \
	libLLVMTarget \
	libLLVMTransformsUtils \
	libLLVMVectorize \
	libLLVMARMCodeGen \
	libLLVMMipsCodeGen \
	libLLVMPowerPCCodeGen \
	libLLVMSparcCodeGen \
	libLLVMX86CodeGen

SUBDIR+= .WAIT

SUBDIR+= \
	libLLVMARMAsmParser \
	libLLVMARMAsmPrinter \
	libLLVMARMDisassembler \
	libLLVMARMMCTargetDesc \
	libLLVMARMTargetInfo

SUBDIR+= \
	libLLVMMipsAsmParser \
	libLLVMMipsAsmPrinter \
	libLLVMMipsMCTargetDesc \
	libLLVMMipsTargetInfo

SUBDIR+= \
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
	libLLVMObject \
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
	libclangRewrite \
	libclangSema \
	libclangSerialization \
	libclangStaticAnalyzerCheckers \
	libclangStaticAnalyzerCore \
	.WAIT \
	libclangStaticAnalyzerFrontend

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libclangTooling
.endif

.include <bsd.subdir.mk>
