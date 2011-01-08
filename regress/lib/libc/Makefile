#	$NetBSD: Makefile,v 1.77 2011/01/08 07:02:17 pgoyette Exp $

SUBDIR+= citrus divrem getaddrinfo int_fmtio locale regex

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
