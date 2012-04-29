#	$NetBSD: Makefile,v 1.5 2012/04/29 19:19:04 joerg Exp $

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
	macho-dumpx \
	opt
.endif

.include <bsd.subdir.mk>
