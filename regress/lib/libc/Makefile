#	$NetBSD: Makefile,v 1.10 1999/02/04 18:56:08 is Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp time
.if ($(MACHINE_ARCH} != "arm32" && ${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
