#	$NetBSD: Makefile,v 1.1 2013/09/02 14:34:56 joerg Exp $

LIB=	lldCore

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/Core

SRCS+=	DefinedAtom.cpp \
	Error.cpp \
	File.cpp \
	InputFiles.cpp \
	LinkingContext.cpp \
	PassManager.cpp \
	Resolver.cpp \
	SymbolTable.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
