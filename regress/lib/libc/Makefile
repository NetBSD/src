#	$NetBSD: Makefile,v 1.67 2010/12/31 04:10:41 pgoyette Exp $

SUBDIR+= atexit citrus clone context db \
	divrem getaddrinfo hsearch inet int_fmtio locale md5sha \
	nsdispatch pty regex rpc servent stdlib strptime sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
