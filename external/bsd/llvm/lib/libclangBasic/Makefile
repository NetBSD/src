#	$NetBSD: Makefile,v 1.12.2.1 2015/06/04 20:05:45 snj Exp $

LIB=	clangBasic

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Basic

SRCS+=	Attributes.cpp \
	Builtins.cpp \
	CharInfo.cpp \
	Diagnostic.cpp \
	DiagnosticIDs.cpp \
	FileManager.cpp \
	FileSystemStatCache.cpp \
	IdentifierTable.cpp \
	LangOptions.cpp \
	Module.cpp \
	ObjCRuntime.cpp \
	OpenMPKinds.cpp \
	OperatorPrecedence.cpp \
	SanitizerBlacklist.cpp \
	Sanitizers.cpp \
	SourceLocation.cpp \
	SourceManager.cpp \
	TargetInfo.cpp \
	Targets.cpp \
	TokenKinds.cpp \
	Version.cpp \
	VersionTuple.cpp \
	VirtualFileSystem.cpp \
	Warnings.cpp

CPPFLAGS.Version.cpp=	-DSVN_REVISION=\"${CLANG_REVISION}\" \
			-DLLVM_REVISION=\"${LLVM_REVISION}\"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
