#	$NetBSD: Makefile,v 1.1.2.2 2018/11/26 01:52:31 pgoyette Exp $

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR= x86
.endif

.include <bsd.own.mk>

INCSDIR= /usr/include/dev/nvmm

INCS=	nvmm.h nvmm_ioctl.h

.include <bsd.kinc.mk>
