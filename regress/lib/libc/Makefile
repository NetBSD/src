#	$NetBSD: Makefile,v 1.12 1999/08/30 17:03:16 mycroft Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp time
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
