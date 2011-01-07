#	$NetBSD: Makefile,v 1.75 2011/01/07 02:51:38 pgoyette Exp $

SUBDIR+= citrus db divrem getaddrinfo int_fmtio locale regex rpc

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
