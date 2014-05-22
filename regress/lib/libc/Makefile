#	$NetBSD: Makefile,v 1.80.4.2 2014/05/22 11:37:18 yamt Exp $

SUBDIR+= citrus divrem

.include <bsd.own.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
