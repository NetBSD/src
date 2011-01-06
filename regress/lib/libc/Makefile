#	$NetBSD: Makefile,v 1.74 2011/01/06 17:22:14 pgoyette Exp $

SUBDIR+= citrus db divrem getaddrinfo int_fmtio locale regex rpc sys

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
