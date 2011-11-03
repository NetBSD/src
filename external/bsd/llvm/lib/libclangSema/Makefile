#	$NetBSD: Makefile,v 1.6 2011/11/03 17:52:54 joerg Exp $

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
	MultiInitializer.cpp \
	Scope.cpp \
	Sema.cpp \
	SemaAccess.cpp \
	SemaAttr.cpp \
	SemaCXXScopeSpec.cpp \
	SemaCast.cpp \
	SemaChecking.cpp \
	SemaCodeComplete.cpp \
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
