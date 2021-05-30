#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:45 joerg Exp $

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
