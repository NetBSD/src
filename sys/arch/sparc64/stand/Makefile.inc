#	$NetBSD: Makefile.inc,v 1.1.1.1.20.1 1999/12/27 18:34:02 wrstuden Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/sparc64/include machine)
.endif

CLEANFILES+=	machine
