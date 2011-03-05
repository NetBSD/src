#	$NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:08:48 bouyer Exp $

LIB=	clangStaticAnalyzerFrontend

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/StaticAnalyzer/Frontend

SRCS+=	AnalysisConsumer.cpp \
	CheckerRegistration.cpp \
	FrontendActions.cpp

CHECKERS_OBJDIR!=	cd ${.CURDIR}/../libclangStaticAnalyzerCheckers && ${PRINTOBJDIR}
CPPFLAGS+=	-I${CHECKERS_OBJDIR}

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
