#	$NetBSD: Makefile,v 1.3.4.2 2014/05/22 16:14:46 yamt Exp $

LIB=	lldCore

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/Core

SRCS+=	DefinedAtom.cpp \
	Error.cpp \
	File.cpp \
	InputGraph.cpp \
	LinkingContext.cpp \
	PassManager.cpp \
	Resolver.cpp \
	SymbolTable.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
