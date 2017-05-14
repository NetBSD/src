#	$NetBSD: Makefile,v 1.13 2017/05/14 03:42:42 kamil Exp $

.include <bsd.own.mk>

SUBDIR= kern fs uvm net
.if exists(arch/${MACHINE}/Makefile)
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH}/Makefile)
SUBDIR+= arch/${MACHINE_ARCH}
.endif

SUBDIR := ${SUBDIR:O:u}

.include <bsd.subdir.mk>
