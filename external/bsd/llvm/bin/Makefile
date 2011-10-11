#	$NetBSD: Makefile,v 1.3 2011/10/11 13:53:57 joerg Exp $

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
	llvm-ld \
	llvm-link \
	llvm-mc \
	llvm-nm \
	llvm-prof \
	llvm-ranlib \
	llvm-size \
	llvm-stub \
	macho-dumpx \
	opt
.endif

.include <bsd.subdir.mk>
