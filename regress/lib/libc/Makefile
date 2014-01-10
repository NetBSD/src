#	$NetBSD: Makefile,v 1.82 2014/01/10 16:35:11 christos Exp $

SUBDIR+= citrus divrem

.include <bsd.own.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
