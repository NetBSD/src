#	$NetBSD: Makefile,v 1.63 2010/12/26 13:38:08 pgoyette Exp $

SUBDIR+= _setjmp atexit citrus clone context convfp db \
	divrem gen gdtoa getaddrinfo hsearch inet int_fmtio locale md5sha \
	nsdispatch pty randomid regex rpc servent setjmp sigsetjmp \
	stdlib strptime sys time

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if defined(USE_SSP) && (${USE_SSP} == "yes")
SUBDIR+= ssp
.endif
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
