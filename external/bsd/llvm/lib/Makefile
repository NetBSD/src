# $NetBSD: Makefile,v 1.33 2013/09/02 14:34:55 joerg Exp $

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
	libLLVMOption \
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
	libLLVMDebugInfo \
	libLLVMCppBackend \
	libLLVMCppBackendInfo \
	libLLVMExecutionEngine \
	libLLVMHello \
	libLLVMInterpreter \
	libLLVMJIT \
	libLLVMMCJIT \
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
	libclangIndex \
	libclangTooling
.endif

.if ${MKLLD} != "no"
SUBDIR+= \
	liblldCore \
	liblldDriver \
	liblldPasses \
	liblldReaderWriter \
	liblldReaderWriterELF \
	liblldReaderWriterELFHexagon \
	liblldReaderWriterELFPPC \
	liblldReaderWriterELFX86 \
	liblldReaderWriterELFX86_64 \
	liblldReaderWriterMachO \
	liblldReaderWriterNative \
	liblldReaderWriterPECOFF \
	liblldReaderWriterYAML
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
