#	$NetBSD: Makefile,v 1.80.4.1 2012/04/17 00:05:37 yamt Exp $

SUBDIR+= citrus divrem

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
