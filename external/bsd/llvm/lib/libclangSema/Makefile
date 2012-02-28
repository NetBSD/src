#	$NetBSD: Makefile,v 1.8 2012/02/28 17:09:31 joerg Exp $

LIB=	clangSema

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Sema

SRCS+=	AnalysisBasedWarnings.cpp \
	AttributeList.cpp \
	CodeCompleteConsumer.cpp \
	DeclSpec.cpp \
	DelayedDiagnostic.cpp \
	IdentifierResolver.cpp \
	JumpDiagnostics.cpp \
	Scope.cpp \
	Sema.cpp \
	SemaAccess.cpp \
	SemaAttr.cpp \
	SemaCXXScopeSpec.cpp \
	SemaCast.cpp \
	SemaChecking.cpp \
	SemaCodeComplete.cpp \
	SemaConsumer.cpp \
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
	SemaObjCProperty.cpp \
	SemaOverload.cpp \
	SemaPseudoObject.cpp \
	SemaStmt.cpp \
	SemaTemplate.cpp \
	SemaTemplateDeduction.cpp \
	SemaTemplateInstantiate.cpp \
	SemaTemplateInstantiateDecl.cpp \
	SemaTemplateVariadic.cpp \
	SemaType.cpp \
	TargetAttributesSema.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
