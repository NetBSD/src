#	$NetBSD: Makefile,v 1.79 2011/01/11 16:34:27 pgoyette Exp $

SUBDIR+= citrus divrem int_fmtio locale

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
