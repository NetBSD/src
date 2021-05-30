#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:57 joerg Exp $

LIB=	clangSema

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Sema

SRCS+=	AnalysisBasedWarnings.cpp \
	CodeCompleteConsumer.cpp \
	DeclSpec.cpp \
	DelayedDiagnostic.cpp \
	IdentifierResolver.cpp \
	JumpDiagnostics.cpp \
	MultiplexExternalSemaSource.cpp \
	ParsedAttr.cpp \
	Scope.cpp \
	ScopeInfo.cpp \
	Sema.cpp \
	SemaAccess.cpp \
	SemaAttr.cpp \
	SemaAvailability.cpp \
	SemaCUDA.cpp \
	SemaCXXScopeSpec.cpp \
	SemaCast.cpp \
	SemaChecking.cpp \
	SemaCodeComplete.cpp \
	SemaConcept.cpp \
	SemaConsumer.cpp \
	SemaCoroutine.cpp \
	SemaDecl.cpp \
	SemaDeclAttr.cpp \
	SemaDeclCXX.cpp \
	SemaDeclObjC.cpp \
	SemaExceptionSpec.cpp \
	SemaExpr.cpp \
	SemaExprCXX.cpp \
	SemaExprObjC.cpp \
	SemaExprMember.cpp \
	SemaFixItUtils.cpp \
	SemaInit.cpp \
	SemaLambda.cpp \
	SemaLookup.cpp \
	SemaModule.cpp \
	SemaObjCProperty.cpp \
	SemaOpenMP.cpp \
	SemaOverload.cpp \
	SemaPseudoObject.cpp \
	SemaStmtAsm.cpp \
	SemaStmtAttr.cpp \
	SemaStmt.cpp \
	SemaSYCL.cpp \
	SemaTemplate.cpp \
	SemaTemplateDeduction.cpp \
	SemaTemplateInstantiate.cpp \
	SemaTemplateInstantiateDecl.cpp \
	SemaTemplateVariadic.cpp \
	SemaType.cpp \
	TypeLocBuilder.cpp

CLANG_TABLEGEN_SRC=		OpenCLBuiltins.td
CLANG_TABLEGEN_OUTPUT= \
	OpenCLBuiltins.inc|-gen-clang-opencl-builtins

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
