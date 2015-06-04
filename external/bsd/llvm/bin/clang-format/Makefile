#	$NetBSD: Makefile,v 1.3.2.1 2015/06/04 20:04:22 snj Exp $

PROG_CXX=	clang-format
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/tools/clang-format

SRCS=	ClangFormat.cpp

CLANG_LIBS+= \
	clangFormat \
	clangToolingCore \
	clangTooling \
	clangFrontend \
	clangSerialization \
	clangDriver \
	clangParse \
	clangRewrite \
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
	Option \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
