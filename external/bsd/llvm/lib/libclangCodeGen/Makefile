#	$NetBSD: Makefile,v 1.9.2.1 2014/08/10 07:09:10 tls Exp $

LIB=	clangCodeGen

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CodeGen

SRCS+=	BackendUtil.cpp \
	CGAtomic.cpp \
	CGBlocks.cpp \
	CGBuiltin.cpp \
	CGCUDANV.cpp \
	CGCUDARuntime.cpp \
	CGCall.cpp \
	CGClass.cpp \
	CGCleanup.cpp \
	CGCXX.cpp \
	CGCXXABI.cpp \
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
	CGRecordLayoutBuilder.cpp \
	CGRTTI.cpp \
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
	ItaniumCXXABI.cpp \
	MicrosoftCXXABI.cpp \
	MicrosoftRTTI.cpp \
	ModuleBuilder.cpp \
	TargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
