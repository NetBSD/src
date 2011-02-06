#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:05 joerg Exp $

LIB=	clangAST

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/AST

SRCS+=	APValue.cpp \
	ASTConsumer.cpp \
	ASTContext.cpp \
	ASTDiagnostic.cpp \
	ASTImporter.cpp \
	AttrImpl.cpp \
	CXXInheritance.cpp \
	Decl.cpp \
	DeclarationName.cpp \
	DeclBase.cpp \
	DeclCXX.cpp \
	DeclFriend.cpp \
	DeclGroup.cpp \
	DeclObjC.cpp \
	DeclPrinter.cpp \
	DeclTemplate.cpp \
	DumpXML.cpp \
	Expr.cpp \
	ExprClassification.cpp \
	ExprConstant.cpp \
	ExprCXX.cpp \
	InheritViz.cpp \
	ItaniumCXXABI.cpp \
	ItaniumMangle.cpp \
	Mangle.cpp \
	MicrosoftCXXABI.cpp \
	MicrosoftMangle.cpp \
	NestedNameSpecifier.cpp \
	ParentMap.cpp \
	RecordLayout.cpp \
	RecordLayoutBuilder.cpp \
	Stmt.cpp \
	StmtDumper.cpp \
	StmtIterator.cpp \
	StmtPrinter.cpp \
	StmtProfile.cpp \
	StmtViz.cpp \
	TemplateBase.cpp \
	TemplateName.cpp \
	Type.cpp \
	TypeLoc.cpp \
	TypePrinter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
