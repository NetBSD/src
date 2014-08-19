#	$NetBSD: Makefile,v 1.81.2.1 2014/08/20 00:02:23 tls Exp $

SUBDIR+= citrus divrem

.include <bsd.own.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
