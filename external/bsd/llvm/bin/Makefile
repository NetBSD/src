#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:39 bouyer Exp $

SUBDIR=	clang \
	tblgen

.if defined(LLVM_DEVELOPER)
SUBDIR+= \
	bugpoint \
	llc \
	lli \
	llvm-ar \
	llvm-as \
	llvm-bcanalyzer \
	llvm-diff \
	llvm-dis \
	llvm-extract \
	llvm-ld \
	llvm-link \
	llvm-mc \
	llvm-nm \
	llvm-prof \
	llvm-ranlib \
	llvm-stub \
	llvmc \
	macho-dumpx \
	opt
.endif

.include <bsd.subdir.mk>
