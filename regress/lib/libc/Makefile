#	$NetBSD: Makefile,v 1.9 1998/01/21 05:47:18 mycroft Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp time
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
