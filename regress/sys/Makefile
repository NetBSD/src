#	$NetBSD: Makefile,v 1.7 1997/10/02 11:06:30 veego Exp $

SUBDIR= kern
.if exists(arch/${MACHINE})
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
