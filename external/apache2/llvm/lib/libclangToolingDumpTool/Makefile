#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:58 joerg Exp $

LIB=	clangToolingDumpTool

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Tooling/DumpTool

SRCS+=	ASTSrcLocProcessor.cpp \
	ClangSrcLocDump.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
