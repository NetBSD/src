#	$NetBSD: Makefile,v 1.12 2013/09/29 12:11:59 joerg Exp $

.include <bsd.own.mk>

SUBDIR=	clang \
	clang-tblgen \
	tblgen

.if ${MKLLD} != "no"
SUBDIR+=	\
	lld
.endif

.if ${MKLLDB} != "no"
SUBDIR+=	\
	lldb
.endif

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
	llvm-readobj \
	llvm-size \
	llvm-symbolizer \
	macho-dumpx \
	opt
.endif

.include <bsd.subdir.mk>
