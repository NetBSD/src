#	$NetBSD: Makefile.inc,v 1.2 1999/11/25 13:25:09 pk Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine)
.endif

CLEANFILES+=	machine
