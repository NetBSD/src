#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:13 cjep Exp $

LIB=	clangStaticAnalyzerFrontend

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Frontend

SRCS+=	AnalysisConsumer.cpp \
	AnalyzerHelpFlags.cpp \
	CheckerRegistry.cpp \
	CreateCheckerManager.cpp \
	FrontendActions.cpp \
	ModelConsumer.cpp \
	ModelInjector.cpp

CHECKERS_OBJDIR!=	cd ${.CURDIR}/../libclangStaticAnalyzerCheckers && ${PRINTOBJDIR}
CPPFLAGS+=	-I${CHECKERS_OBJDIR}

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
