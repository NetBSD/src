#	$NetBSD: Makefile,v 1.5 1995/04/20 22:38:32 cgd Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif

regress: _SUBDIRUSE

install:

.include <bsd.subdir.mk>
