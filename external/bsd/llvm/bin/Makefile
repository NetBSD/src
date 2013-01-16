#	$NetBSD: Makefile,v 1.3.2.3 2013/01/16 05:27:58 yamt Exp $

.include <bsd.own.mk>

SUBDIR=	clang \
	clang-tblgen \
	tblgen

.if defined(LLVM_DEVELOPER)
SUBDIR+= \
	bugpoint \
	llc \
	lli \
	llvm-ar \
	llvm-as \
	llvm-bcanalyzer \
	llvm-cov \
	llvm-diff \
	llvm-dis \
	llvm-dwarfdump \
	llvm-extract \
	llvm-link \
	llvm-mc \
	llvm-nm \
	llvm-prof \
	llvm-ranlib \
	llvm-readobj \
	llvm-size \
	llvm-symbolizer \
	macho-dumpx \
	opt
.endif

.include <bsd.subdir.mk>
