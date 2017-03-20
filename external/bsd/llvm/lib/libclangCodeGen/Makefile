#	$NetBSD: Makefile,v 1.13.2.1 2017/03/20 06:56:02 pgoyette Exp $

LIB=	clangCodeGen

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CodeGen

SRCS+=	BackendUtil.cpp \
	CGAtomic.cpp \
	CGBlocks.cpp \
	CGBuiltin.cpp \
	CGCUDABuiltin.cpp \
	CGCUDANV.cpp \
	CGCUDARuntime.cpp \
	CGCXX.cpp \
	CGCXXABI.cpp \
	CGCall.cpp \
	CGClass.cpp \
	CGCleanup.cpp \
	CGCoroutine.cpp \
	CGDebugInfo.cpp \
	CGDecl.cpp \
	CGDeclCXX.cpp \
	CGException.cpp \
	CGExpr.cpp \
	CGExprAgg.cpp \
	CGExprComplex.cpp \
	CGExprConstant.cpp \
	CGExprCXX.cpp \
	CGExprScalar.cpp \
	CGLoopInfo.cpp \
	CGObjC.cpp \
	CGObjCGNU.cpp \
	CGObjCMac.cpp \
	CGObjCRuntime.cpp \
	CGOpenCLRuntime.cpp \
	CGOpenMPRuntime.cpp \
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
	CoverageMappingGen.cpp \
	ItaniumCXXABI.cpp \
	MicrosoftCXXABI.cpp \
	ModuleBuilder.cpp \
	ObjectFilePCHContainerOperations.cpp \
	SanitizerMetadata.cpp \
	SwiftCallingConv.cpp \
	TargetInfo.cpp \
	VarBypassDetector.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
