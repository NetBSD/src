#	$NetBSD: Makefile,v 1.1.6.2 2019/06/10 22:07:14 christos Exp $

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR= x86
.endif

.include <bsd.own.mk>

INCSDIR= /usr/include/dev/nvmm

INCS=	nvmm.h nvmm_ioctl.h

.include <bsd.kinc.mk>
