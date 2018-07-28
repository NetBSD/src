#	$NetBSD: Makefile,v 1.5.12.1 2018/07/28 04:33:04 pgoyette Exp $

PROG_CXX=	clang-format
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/tools/clang-format

SRCS=	ClangFormat.cpp

CLANG_LIBS+= \
	clangFormat \
	clangToolingCore \
	clangToolingInclusions \
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
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
