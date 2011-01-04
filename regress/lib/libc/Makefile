#	$NetBSD: Makefile,v 1.72 2011/01/04 14:22:06 pgoyette Exp $

SUBDIR+= citrus db divrem getaddrinfo int_fmtio locale \
	nsdispatch regex rpc sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
