#	$NetBSD: Makefile,v 1.30 2002/07/15 22:04:00 christos Exp $

SUBDIR+= _setjmp clone db div divrem \
	gen getaddrinfo hsearch int_fmtio md5sha popen regex rpc \
	setjmp sigsetjmp stdlib string sys time
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
