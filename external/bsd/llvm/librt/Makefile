.include <bsd.own.mk>

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
	libLLVMVectorize \
	libLLVMAMDGPUCodeGen \

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

SUBDIR+= \
	.WAIT \
	libLLVMAMDGPUMCTargetDesc \
	libLLVMAMDGPUAsmPrinter \
	libLLVMAMDGPUTargetInfo \
	libLLVMAMDGPUAsmParser \
	libLLVMAMDGPUUtils

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
