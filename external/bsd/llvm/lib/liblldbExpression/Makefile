#	$NetBSD: Makefile,v 1.1.6.1 2015/06/04 20:05:46 snj Exp $

LIB=	lldbExpression

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/Expression

SRCS+=	ASTDumper.cpp \
	ASTResultSynthesizer.cpp \
	ASTStructExtractor.cpp \
	ClangASTSource.cpp \
	ClangExpressionDeclMap.cpp \
	ClangExpressionParser.cpp \
	ClangExpressionVariable.cpp \
	ClangFunction.cpp \
	ClangModulesDeclVendor.cpp \
	ClangPersistentVariables.cpp \
	ClangUserExpression.cpp \
	ClangUtilityFunction.cpp \
	DWARFExpression.cpp \
	ExpressionSourceCode.cpp \
	IRDynamicChecks.cpp \
	IRExecutionUnit.cpp \
	IRForTarget.cpp \
	IRInterpreter.cpp \
	IRMemoryMap.cpp \
	Materializer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
