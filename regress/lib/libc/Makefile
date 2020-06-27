#	$NetBSD: Makefile,v 1.83 2020/06/27 11:06:43 mlelstv Exp $

SUBDIR+= divrem

.include <bsd.own.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
