#	$NetBSD: Makefile,v 1.2 2013/10/14 01:37:40 joerg Exp $

LIB=	lldCore

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/Core

SRCS+=	DefinedAtom.cpp \
	Error.cpp \
	File.cpp \
	LinkingContext.cpp \
	PassManager.cpp \
	Resolver.cpp \
	SymbolTable.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
