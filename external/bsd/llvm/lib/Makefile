# $NetBSD: Makefile,v 1.1 2011/02/06 01:13:52 joerg Exp $

SUBDIR=	\
	libLLVMAnalysis \
	libLLVMAsmParser \
	libLLVMAsmPrinter \
	libLLVMBitReader \
	libLLVMBitWriter \
	libLLVMCodeGen \
	libLLVMCore \
	libLLVMInstCombine \
	libLLVMipa \
	libLLVMipo \
	libLLVMMC \
	libLLVMMCDisassembler \
	libLLVMMCParser \
	libLLVMScalarOpts \
	libLLVMSelectionDAG \
	libLLVMSupport \
	libLLVMTarget \
	libLLVMTransformsUtils \
	libLLVMX86CodeGen

SUBDIR+= \
	.WAIT \
	libLLVMX86AsmParser \
	libLLVMX86AsmPrinter \
	libLLVMX86Disassembler \
	libLLVMX86TargetInfo

.if ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libLLVMArchive \
	libLLVMCBackend \
	libLLVMCBackendInfo \
	libLLVMCompilerDriver \
	libLLVMCppBackend \
	libLLVMCppBackendInfo \
	libLLVMExecutionEngine \
	libLLVMHello \
	libLLVMInstrumentation \
	libLLVMInterpreter \
	libLLVMJIT \
	libLLVMLinker \
	libLLVMMCJIT \
	libLLVMObject
.endif

SUBDIR+= \
	libclangAnalysis \
	libclangAST \
	libclangBasic \
	libclangCodeGen \
	libclangDriver \
	libclangFrontend \
	libclangFrontendTool \
	libclangIndex \
	libclangLex \
	libclangParse \
	libclangRewrite \
	libclangSema \
	libclangSerialization \
	libclangStaticAnalyzer \
	libclangStaticAnalyzerCheckers

.include <bsd.subdir.mk>
