#	$NetBSD: Makefile,v 1.11.2.1 2000/02/22 21:58:37 he Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp time
.if (${MACHINE_ARCH} != "arm32" && ${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "powerpc")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
