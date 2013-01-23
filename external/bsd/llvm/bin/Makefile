#	$NetBSD: Makefile,v 1.7 2013/01/23 15:02:54 joerg Exp $

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
	llvm-objdump \
	llvm-prof \
	llvm-ranlib \
	llvm-readobj \
	llvm-size \
	llvm-symbolizer \
	macho-dumpx \
	opt
.endif

.include <bsd.subdir.mk>
