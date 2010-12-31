#	$NetBSD: Makefile,v 1.68 2010/12/31 14:37:37 pgoyette Exp $

SUBDIR+= atexit citrus db divrem getaddrinfo int_fmtio locale md5sha \
	nsdispatch regex rpc servent stdlib sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
