#	$NetBSD: Makefile,v 1.15 2017/05/14 04:17:25 kamil Exp $

.include <bsd.own.mk>

SUBDIR= kern fs
.if exists(arch/${MACHINE}/Makefile)
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH}/Makefile)
SUBDIR+= arch/${MACHINE_ARCH}
.endif

SUBDIR := ${SUBDIR:O:u}

.include <bsd.subdir.mk>
