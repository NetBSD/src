#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:56 cjep Exp $

.include <bsd.init.mk>

SUBDIR=	include \

.if ${MKLLVMRT} != "no"
SUBDIR+= \
	librt
.endif

.if ${MKLLVM} != "no"
SUBDIR+= \
	lib \
	.WAIT \
	bin
.endif

prepare-import:
	cd ${LLVM_TOPLEVEL}/dist && rm -rf */test */unittests */www

.include "Makefile.inc"

.include <bsd.subdir.mk>
