#	$NetBSD: Makefile,v 1.4 2011/12/11 14:58:50 joerg Exp $

LIB=	clangBasic

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Basic

SRCS+=	Builtins.cpp \
	ConvertUTF.c \
	Diagnostic.cpp \
	DiagnosticIDs.cpp \
	FileManager.cpp \
	FileSystemStatCache.cpp \
	IdentifierTable.cpp \
	LangOptions.cpp \
	Module.cpp \
	SourceLocation.cpp \
	SourceManager.cpp \
	TargetInfo.cpp \
	Targets.cpp \
	TokenKinds.cpp \
	Version.cpp \
	VersionTuple.cpp

CPPFLAGS.version.cpp=	-DSVN_REVISON=\"${CLANG_REVISION}\"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
