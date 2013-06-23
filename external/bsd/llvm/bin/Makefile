#	$NetBSD: Makefile,v 1.5.2.3 2013/06/23 06:28:21 tls Exp $

.include <bsd.own.mk>

SUBDIR=	clang \
	clang-tblgen \
	tblgen

.if ${MKMCLINKER} != "no"
SUBDIR+=	\
	llvm-mcld
.endif

.if defined(LLVM_DEVELOPER)
SUBDIR+= \
	bugpoint \
	clang-format \
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
