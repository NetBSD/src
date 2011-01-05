#	$NetBSD: Makefile,v 1.73 2011/01/05 21:20:25 pgoyette Exp $

SUBDIR+= citrus db divrem getaddrinfo int_fmtio locale regex rpc sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
