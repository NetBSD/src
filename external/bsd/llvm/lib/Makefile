# $NetBSD: Makefile,v 1.39.2.1 2014/08/10 07:09:08 tls Exp $

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
	libLLVMProfileData \
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
	libLLVMPowerPCDisassembler \
	libLLVMPowerPCMCTargetDesc \
	libLLVMPowerPCTargetInfo

SUBDIR+= \
	libLLVMSparcAsmParser \
	libLLVMSparcAsmPrinter \
	libLLVMSparcDisassembler \
	libLLVMSparcMCTargetDesc \
	libLLVMSparcTargetInfo

SUBDIR+= \
	libLLVMX86AsmParser \
	libLLVMX86AsmPrinter \
	libLLVMX86Disassembler \
	libLLVMX86MCTargetDesc \
	libLLVMX86TargetInfo \
	libLLVMX86Utils

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && (${LLVM_DEVELOPER:U} == "yes" || ${MKLLDB} == "yes")
SUBDIR+= \
	libLLVMExecutionEngine \
	libLLVMJIT \
	libLLVMMCJIT \
	libLLVMRuntimeDyld
.endif

.if (${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes") || ${MKLLD} == "yes"
SUBDIR+= \
	libLLVMDebugInfo
.endif

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libLLVMCppBackend \
	libLLVMCppBackendInfo \
	libLLVMHello \
	libLLVMInterpreter \
	libLLVMLTO \
	libLLVMLineEditor
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
	liblldReaderWriterELFMips \
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

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${MKLLDB} == "yes"
SUBDIR+= \
	liblldbAPI \
	liblldbBreakpoint \
	liblldbCommands \
	liblldbCore \
	liblldbDataFormatters \
	liblldbExpression \
	liblldbHostCommon \
	liblldbHostNetBSD \
	liblldbInterpreter \
	liblldbLog \
	liblldbPluginABIMacOSX_arm \
	liblldbPluginABIMacOSX_arm64 \
	liblldbPluginABIMacOSX_i386 \
	liblldbPluginABISysV_x86_64 \
	liblldbPluginAppleObjCRuntime \
	liblldbPluginCXXItaniumABI \
	liblldbPluginDisassemblerLLVM \
	liblldbPluginDynamicLoaderMacOSXDYLD \
	liblldbPluginDynamicLoaderPosixDYLD \
	liblldbPluginDynamicLoaderStatic \
	liblldbPluginInstructionARM \
	liblldbPluginInstructionARM64 \
	liblldbPluginJITLoaderGDB \
	liblldbPluginObjectContainerBSDArchive \
	liblldbPluginObjectContainerMachOArchive \
	liblldbPluginObjectFileELF \
	liblldbPluginObjectFileJIT \
	liblldbPluginObjectFileMachO \
	liblldbPluginObjectFilePECOFF \
	liblldbPluginPlatformNetBSD \
	liblldbPluginPlatformGDB \
	liblldbPluginPlatformLinux \
	liblldbPluginPlatformMacOSX \
	liblldbPluginPlatformPOSIX \
	liblldbPluginPlatformWindows \
	liblldbPluginProcessElfCore \
	liblldbPluginProcessNetBSD \
	liblldbPluginProcessGDBRemote \
	liblldbPluginProcessMachCore \
	liblldbPluginProcessPOSIX \
	liblldbPluginProcessUtility \
	liblldbPluginSymbolFileDWARF \
	liblldbPluginSymbolFileSymtab \
	liblldbPluginSymbolVendorELF \
	liblldbPluginSystemRuntimeMacOSX \
	liblldbPluginUnwindAssemblyInstEmulation \
	liblldbPluginUnwindAssemblyX86 \
	liblldbSymbol \
	liblldbTarget \
	liblldbUtility
.endif

.include <bsd.subdir.mk>
