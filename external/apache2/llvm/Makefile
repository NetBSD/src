#	$NetBSD: Makefile,v 1.1 2019/11/11 22:44:57 joerg Exp $

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

.include "Makefile.inc"

.include <bsd.subdir.mk>
