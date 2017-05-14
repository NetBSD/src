#	$NetBSD: Makefile,v 1.16 2017/05/14 04:26:40 kamil Exp $

.include <bsd.own.mk>

SUBDIR= kern
.if exists(arch/${MACHINE}/Makefile)
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH}/Makefile)
SUBDIR+= arch/${MACHINE_ARCH}
.endif

SUBDIR := ${SUBDIR:O:u}

.include <bsd.subdir.mk>
