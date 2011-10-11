#	$NetBSD: Makefile,v 1.3 2011/10/11 13:54:00 joerg Exp $

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
