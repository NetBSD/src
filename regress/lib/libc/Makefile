#	$Id: Makefile,v 1.4 1995/04/18 19:24:21 jtc Exp $

SUBDIR+= _setjmp db regex setjmp sigsetjmp
.if (${MACHINE_ARCH} != "vax")
SUBDIR+= ieeefp
.endif

regress: _SUBDIRUSE

install:

.include <bsd.subdir.mk>
