#	$NetBSD: Makefile,v 1.64 2010/12/27 19:39:09 pgoyette Exp $

SUBDIR+= atexit citrus clone context convfp db \
	divrem gen gdtoa getaddrinfo hsearch inet int_fmtio locale md5sha \
	nsdispatch pty randomid regex rpc servent stdlib strptime sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ldexp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
