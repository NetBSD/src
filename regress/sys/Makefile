#	$NetBSD: Makefile,v 1.6 1997/09/30 23:15:45 is Exp $

SUBDIR= kern
.if exists(arch/${MACHINE})
SUBDIR= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH})
SUBDIR= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
