#	$NetBSD: Makefile,v 1.36 2003/09/26 18:13:36 christos Exp $

SUBDIR+= _setjmp atexit basename clone convfp db dirname div divrem \
	gen getaddrinfo hsearch int_fmtio locale md5sha popen regex rpc \
	setjmp siginfo sigsetjmp stdio stdlib string sys time
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
