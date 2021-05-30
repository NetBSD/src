#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:56 joerg Exp $

LIB=	clangCodeGen

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CodeGen

SRCS+=	BackendUtil.cpp \
	CGAtomic.cpp \
	CGBlocks.cpp \
	CGBuiltin.cpp \
	CGCall.cpp \
	CGClass.cpp \
	CGCleanup.cpp \
	CGCoroutine.cpp \
	CGCUDANV.cpp \
	CGCUDARuntime.cpp \
	CGCXXABI.cpp \
	CGCXX.cpp \
	CGDebugInfo.cpp \
	CGDecl.cpp \
	CGDeclCXX.cpp \
	CGException.cpp \
	CGExprAgg.cpp \
	CGExprComplex.cpp \
	CGExprConstant.cpp \
	CGExpr.cpp \
	CGExprCXX.cpp \
	CGExprScalar.cpp \
	CGGPUBuiltin.cpp \
	CGLoopInfo.cpp \
	CGNonTrivialStruct.cpp \
	CGObjC.cpp \
	CGObjCGNU.cpp \
	CGObjCMac.cpp \
	CGObjCRuntime.cpp \
	CGOpenCLRuntime.cpp \
	CGOpenMPRuntimeAMDGCN.cpp \
	CGOpenMPRuntime.cpp \
	CGOpenMPRuntimeGPU.cpp \
	CGOpenMPRuntimeNVPTX.cpp \
	CGRecordLayoutBuilder.cpp \
	CGStmt.cpp \
	CGStmtOpenMP.cpp \
	CGVTables.cpp \
	CGVTT.cpp \
	CodeGenABITypes.cpp \
	CodeGenAction.cpp \
	CodeGenFunction.cpp \
	CodeGenModule.cpp \
	CodeGenPGO.cpp \
	CodeGenTBAA.cpp \
	CodeGenTypes.cpp \
	ConstantInitBuilder.cpp \
	CoverageMappingGen.cpp \
	ItaniumCXXABI.cpp \
	MacroPPCallbacks.cpp \
	MicrosoftCXXABI.cpp \
	ModuleBuilder.cpp \
	ObjectFilePCHContainerOperations.cpp \
	PatternInit.cpp \
	SanitizerMetadata.cpp \
	SwiftCallingConv.cpp \
	TargetInfo.cpp \
	VarBypassDetector.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
