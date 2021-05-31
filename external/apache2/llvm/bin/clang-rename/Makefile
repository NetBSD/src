#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:57 cjep Exp $

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
	BitstreamReader \
	IR \
	FrontendOpenMP \
	MCParser \
	MC \
	BinaryFormat \
	Option \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
