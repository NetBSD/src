#	$NetBSD: Makefile,v 1.76 2011/01/07 15:08:40 pgoyette Exp $

SUBDIR+= citrus divrem getaddrinfo int_fmtio locale regex rpc

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
