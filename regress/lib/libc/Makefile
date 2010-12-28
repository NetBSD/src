#	$NetBSD: Makefile,v 1.66 2010/12/28 17:35:25 pgoyette Exp $

SUBDIR+= atexit citrus clone context convfp db \
	divrem gdtoa getaddrinfo hsearch inet int_fmtio locale md5sha \
	nsdispatch pty randomid regex rpc servent stdlib strptime sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if (${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
