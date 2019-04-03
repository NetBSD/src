#	$NetBSD: Makefile,v 1.19 2019/04/03 15:12:45 joerg Exp $

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
	llvm-symbolizer \
	llvm-xray \
	opt \
	sancov \
	sanstats
.endif

.include <bsd.subdir.mk>
