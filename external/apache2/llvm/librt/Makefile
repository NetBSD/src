#	$NetBSD: Makefile,v 1.5 2023/07/15 21:14:01 rjs Exp $

.include <bsd.own.mk>

BUILD_RADEON=0
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE} == "evbarm"
BUILD_RADEON=1
.endif

SUBDIR=	libLLVMAnalysis \
	libLLVMAsmParser \
	libLLVMAsmPrinter \
	libLLVMBinaryFormat \
	libLLVMBitReader \
	libLLVMBitWriter \
	libLLVMBitstreamReader \
	libLLVMCodeGen \
	libLLVMDemangle \
	libLLVMDebugInfoCodeView \
	libLLVMDebugInfoDWARF \
	libLLVMExecutionEngine \
	libLLVMExecutionEngineOrcShared \
	libLLVMExecutionEngineOrcTargetProcess \
	libLLVMFrontendOpenMP \
	libLLVMGlobalISel \
	libLLVMInstCombine \
	libLLVMInstrumentation \
	libLLVMipo \
	libLLVMIR \
	libLLVMIRReader \
	libLLVMLinker \
	libLLVMMC \
	libLLVMMCDisassembler \
	libLLVMMCJIT \
	libLLVMMCParser \
	libLLVMMIRParser \
	libLLVMObject \
	libLLVMObjCARC \
	libLLVMOrc \
	libLLVMPasses \
	libLLVMProfileData \
	libLLVMRemarks \
	libLLVMRuntimeDyld \
	libLLVMScalarOpts \
	libLLVMSelectionDAG \
	libLLVMSupport \
	libLLVMTarget \
	libLLVMTextAPI \
	libLLVMTransformsAggressiveInstCombine \
	libLLVMTransformsCFGuard \
	libLLVMTransformsCoroutines \
	libLLVMTransformsUtils \
	libLLVMVectorize

.if ${BUILD_RADEON} == 1
SUBDIR+= \
	libLLVMAMDGPUCodeGen
.endif

.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "aarch64eb"
SUBDIR+= \
	libLLVMAArch64CodeGen
.endif
.if !empty(MACHINE_ARCH:Mearm*) || !empty(MACHINE_ARCH:Marm*)
SUBDIR+= \
	libLLVMARMCodeGen
.endif
.if !empty(MACHINE_ARCH:Mmips*)
SUBDIR+= \
	libLLVMMipsCodeGen
.endif
.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpc64"
SUBDIR+= \
	libLLVMPowerPCCodeGen
.endif
.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
SUBDIR+= \
	libLLVMSparcCodeGen
.endif

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+= \
	libLLVMX86CodeGen
.endif

# All subdirs below here can build together, and all are optional.
SUBDIR+= .WAIT 

.if ${BUILD_RADEON} == 1
SUBDIR+= \
	libLLVMAMDGPUMCTargetDesc \
	libLLVMAMDGPUTargetInfo \
	libLLVMAMDGPUAsmParser \
	libLLVMAMDGPUDisassembler \
	libLLVMAMDGPUUtils
.endif

.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "aarch64eb"
SUBDIR+= \
	libLLVMAArch64AsmParser \
	libLLVMAArch64Disassembler \
	libLLVMAArch64MCTargetDesc \
	libLLVMAArch64TargetInfo \
	libLLVMAArch64Utils
.endif

.if !empty(MACHINE_ARCH:Mearm*) || !empty(MACHINE_ARCH:Marm*)
SUBDIR+= \
	libLLVMARMAsmParser \
	libLLVMARMDisassembler \
	libLLVMARMMCTargetDesc \
	libLLVMARMTargetInfo \
	libLLVMARMUtils
.endif

.if !empty(MACHINE_ARCH:Mmips*)
SUBDIR+= \
	libLLVMMipsAsmParser \
	libLLVMMipsDisassembler \
	libLLVMMipsMCTargetDesc \
	libLLVMMipsTargetInfo
.endif

.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpc64"
SUBDIR+= \
	libLLVMPowerPCAsmParser \
	libLLVMPowerPCDisassembler \
	libLLVMPowerPCMCTargetDesc \
	libLLVMPowerPCTargetInfo
.endif

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
SUBDIR+= \
	libLLVMSparcAsmParser \
	libLLVMSparcDisassembler \
	libLLVMSparcMCTargetDesc \
	libLLVMSparcTargetInfo
.endif

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+= \
	libLLVMX86AsmParser \
	libLLVMX86Disassembler \
	libLLVMX86MCTargetDesc \
	libLLVMX86TargetInfo
.endif

.include <bsd.subdir.mk>
