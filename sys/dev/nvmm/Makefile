#	$NetBSD: Makefile,v 1.1 2018/11/07 07:43:08 maxv Exp $

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR= x86
.endif

.include <bsd.own.mk>

INCSDIR= /usr/include/dev/nvmm

INCS=	nvmm.h nvmm_ioctl.h

.include <bsd.kinc.mk>
