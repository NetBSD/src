#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:52 bouyer Exp $

LIB=	clangCodeGen

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CodeGen

SRCS+=	BackendUtil.cpp \
	CGBlocks.cpp \
	CGBuiltin.cpp \
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
	CGObjC.cpp \
	CGObjCGNU.cpp \
	CGObjCMac.cpp \
	CGRecordLayoutBuilder.cpp \
	CGRTTI.cpp \
	CGStmt.cpp \
	CGTemporaries.cpp \
	CGVTables.cpp \
	CGVTT.cpp \
	CodeGenAction.cpp \
	CodeGenFunction.cpp \
	CodeGenModule.cpp \
	CodeGenTBAA.cpp \
	CodeGenTypes.cpp \
	ItaniumCXXABI.cpp \
	MicrosoftCXXABI.cpp \
	ModuleBuilder.cpp \
	TargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
