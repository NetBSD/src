#	$NetBSD: Makefile,v 1.8 1997/10/11 22:57:49 mycroft Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
