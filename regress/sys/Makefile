#	$NetBSD: Makefile,v 1.9 1999/06/19 00:53:31 thorpej Exp $

SUBDIR= kern fs uvm
.if exists(arch/${MACHINE})
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
