#	$NetBSD: Makefile,v 1.13 1999/09/30 09:29:11 tron Exp $

SUBDIR+= _setjmp db popen regex setjmp sigsetjmp time
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
