#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:53 bouyer Exp $

LIB=	clangIndex

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Index

SRCS+=	ASTLocation.cpp \
	Analyzer.cpp \
	CallGraph.cpp \
	DeclReferenceMap.cpp \
	Entity.cpp \
	GlobalSelector.cpp \
	Handlers.cpp \
	IndexProvider.cpp \
	Indexer.cpp \
	Program.cpp \
	SelectorMap.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
