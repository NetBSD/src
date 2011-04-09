#	$NetBSD: Makefile,v 1.80 2011/04/09 17:55:51 pgoyette Exp $

SUBDIR+= citrus divrem int_fmtio

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
