#	$NetBSD: Makefile.inc,v 1.3 1999/11/25 14:46:08 pk Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/sparc64/include machine)
.endif

CLEANFILES+=	machine
