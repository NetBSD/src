#	$NetBSD: Makefile,v 1.70 2011/01/02 03:59:34 pgoyette Exp $

SUBDIR+= atexit citrus db divrem getaddrinfo int_fmtio locale md5sha \
	nsdispatch regex rpc servent sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
