#	$NetBSD: Makefile,v 1.8 1999/03/10 18:57:06 bouyer Exp $

SUBDIR= kern fs
.if exists(arch/${MACHINE})
SUBDIR+= arch/${MACHINE}
.endif
.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
