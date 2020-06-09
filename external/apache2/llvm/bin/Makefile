#	$NetBSD: Makefile,v 1.2 2020/06/09 21:50:32 kamil Exp $

.include <bsd.own.mk>

SUBDIR=	clang \
	clang-tblgen \
	tblgen \
	llvm-symbolizer

.if defined(LLVM_DEVELOPER)
SUBDIR+= \
	bugpoint \
	clang-format \
	clang-rename \
	llc \
	lli \
	llvm-ar \
	llvm-as \
	llvm-bcanalyzer \
	llvm-cov \
	llvm-cxxdump \
	llvm-cxxfilt \
	llvm-diff \
	llvm-dis \
	llvm-dwarfdump \
	llvm-dwp \
	llvm-extract \
	llvm-link \
	llvm-mc \
	llvm-nm \
	llvm-objdump \
	llvm-profdata \
	llvm-readobj \
	llvm-size \
	llvm-strings \
	llvm-xray \
	opt \
	sancov \
	sanstats
.endif

.include <bsd.subdir.mk>
