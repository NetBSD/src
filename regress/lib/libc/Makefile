#	$NetBSD: Makefile,v 1.31 2002/09/11 14:55:55 minoura Exp $

SUBDIR+= _setjmp clone db div divrem \
	gen getaddrinfo hsearch int_fmtio locale md5sha popen regex rpc \
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
