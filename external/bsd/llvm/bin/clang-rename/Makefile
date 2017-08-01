#	$NetBSD: Makefile,v 1.1 2017/08/01 20:07:37 joerg Exp $

PROG_CXX=	clang-rename
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/tools/clang-rename

SRCS=	ClangRename.cpp

CLANG_LIBS+= \
	clangToolingRefactoring \
	clangToolingCore \
	clangTooling \
	clangFrontend \
	clangIndex \
	clangSerialization \
	clangDriver \
	clangParse \
	clangRewrite \
	clangSema \
	clangEdit \
	clangAnalysis \
	clangASTMatchers \
	clangAST \
	clangLex \
	clangBasic

LLVM_LIBS+= \
	ProfileData \
	BitReader \
	IR \
	MCParser \
	MC \
	Option \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
