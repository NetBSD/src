#	$NetBSD: Makefile,v 1.1 2013/03/24 13:02:17 joerg Exp $

PROG_CXX=	clang-format
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/tools/clang-format

SRCS=	ClangFormat.cpp

CLANG_LIBS+= \
	clangFormat \
	clangTooling \
	clangFrontend \
	clangSerialization \
	clangDriver \
	clangParse \
	clangRewriteFrontend \
	clangRewriteCore \
	clangSema \
	clangEdit \
	clangAnalysis \
	clangAST \
	clangLex \
	clangBasic

LLVM_LIBS+= \
	BitReader \
	MCParser \
	MC \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
