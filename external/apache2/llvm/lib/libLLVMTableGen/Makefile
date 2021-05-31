#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:10 cjep Exp $

LIB=	LLVMTableGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/TableGen

SRCS+=	DetailedRecordsBackend.cpp \
	Error.cpp \
	JSONBackend.cpp \
	Main.cpp \
	Record.cpp \
	SetTheory.cpp \
	StringMatcher.cpp \
	TableGenBackend.cpp \
	TableGenBackendSkeleton.cpp \
	TGLexer.cpp \
	TGParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
