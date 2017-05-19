#	$NetBSD: Makefile,v 1.12.74.1 2017/05/19 00:22:53 pgoyette Exp $

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
