#	$NetBSD: Makefile,v 1.78 2011/01/08 18:42:47 pgoyette Exp $

SUBDIR+= citrus divrem getaddrinfo int_fmtio locale

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
