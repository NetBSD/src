#	$NetBSD: Makefile,v 1.6 2019/06/25 10:30:29 mrg Exp $

.include <bsd.own.mk>

BUILD_RADEON=0
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
BUILD_RADEON=1
.endif

SUBDIR=	libLLVMAnalysis \
	libLLVMAsmParser \
	libLLVMAsmPrinter \
	libLLVMBinaryFormat \
	libLLVMBitReader \
	libLLVMCodeGen \
	libLLVMDebugInfoCodeView \
	libLLVMExecutionEngine \
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
	libLLVMObject \
	libLLVMPasses \
	libLLVMProfileData \
	libLLVMRuntimeDyld \
	libLLVMScalarOpts \
	libLLVMSelectionDAG \
	libLLVMSupport \
	libLLVMTarget \
	libLLVMTransformsAggressiveInstCombine \
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
	libLLVMAMDGPUAsmPrinter \
	libLLVMAMDGPUTargetInfo \
	libLLVMAMDGPUAsmParser \
	libLLVMAMDGPUUtils
.endif

.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "aarch64eb"
SUBDIR+= \
	libLLVMAArch64AsmParser \
	libLLVMAArch64AsmPrinter \
	libLLVMAArch64Disassembler \
	libLLVMAArch64MCTargetDesc \
	libLLVMAArch64TargetInfo \
	libLLVMAArch64Utils
.endif

.if !empty(MACHINE_ARCH:Mearm*) || !empty(MACHINE_ARCH:Marm*)
SUBDIR+= \
	libLLVMARMAsmParser \
	libLLVMARMAsmPrinter \
	libLLVMARMDisassembler \
	libLLVMARMMCTargetDesc \
	libLLVMARMTargetInfo \
	libLLVMARMUtils
.endif

.if !empty(MACHINE_ARCH:Mmips*)
SUBDIR+= \
	libLLVMMipsAsmParser \
	libLLVMMipsAsmPrinter \
	libLLVMMipsDisassembler \
	libLLVMMipsMCTargetDesc \
	libLLVMMipsTargetInfo
.endif

.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpc64"
SUBDIR+= \
	libLLVMPowerPCAsmParser \
	libLLVMPowerPCAsmPrinter \
	libLLVMPowerPCDisassembler \
	libLLVMPowerPCMCTargetDesc \
	libLLVMPowerPCTargetInfo
.endif

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
SUBDIR+= \
	libLLVMSparcAsmParser \
	libLLVMSparcAsmPrinter \
	libLLVMSparcDisassembler \
	libLLVMSparcMCTargetDesc \
	libLLVMSparcTargetInfo
.endif

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+= \
	libLLVMX86AsmParser \
	libLLVMX86AsmPrinter \
	libLLVMX86Disassembler \
	libLLVMX86MCTargetDesc \
	libLLVMX86TargetInfo \
	libLLVMX86Utils
.endif

.include <bsd.subdir.mk>
