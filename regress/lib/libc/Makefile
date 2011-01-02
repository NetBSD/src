#	$NetBSD: Makefile,v 1.71 2011/01/02 22:10:23 pgoyette Exp $

SUBDIR+= atexit citrus db divrem getaddrinfo int_fmtio locale \
	nsdispatch regex rpc sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
