#	$NetBSD: Makefile,v 1.14 2000/08/19 17:53:24 sommerfeld Exp $

SUBDIR+= _setjmp db md5sha popen regex setjmp sigsetjmp time
.if (${MACHINE_ARCH} != "arm32" && ${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ldexp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
