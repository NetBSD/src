#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:14:46 yamt Exp $

LIB=	clangParse

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Parse

SRCS+=	ParseAST.cpp \
	ParseCXXInlineMethods.cpp \
	ParseDecl.cpp \
	ParseDeclCXX.cpp \
	ParseExpr.cpp \
	ParseExprCXX.cpp \
	ParseInit.cpp \
	ParseObjc.cpp \
	ParseOpenMP.cpp \
	ParsePragma.cpp \
	ParseStmt.cpp \
	ParseTemplate.cpp \
	ParseTentative.cpp \
	Parser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
