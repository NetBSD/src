#	$NetBSD: Makefile,v 1.69 2011/01/01 23:59:19 pgoyette Exp $

SUBDIR+= atexit citrus db divrem getaddrinfo int_fmtio locale md5sha \
	nsdispatch regex rpc servent sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
