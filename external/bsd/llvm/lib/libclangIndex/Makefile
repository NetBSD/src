#	$NetBSD: Makefile,v 1.2 2012/04/04 10:42:04 joerg Exp $

LIB=	clangIndex

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Index

SRCS+=	ASTLocation.cpp \
	Analyzer.cpp \
	DeclReferenceMap.cpp \
	Entity.cpp \
	GlobalCallGraph.cpp \
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
