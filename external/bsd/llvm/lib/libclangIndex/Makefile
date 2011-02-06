#	$NetBSD: Makefile,v 1.1 2011/02/06 01:14:08 joerg Exp $

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
