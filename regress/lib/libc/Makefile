#	$NetBSD: Makefile,v 1.7 1997/05/07 16:01:33 mycroft Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

regress: _SUBDIRUSE

.include <bsd.subdir.mk>
