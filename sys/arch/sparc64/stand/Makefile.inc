#	$NetBSD: Makefile.inc,v 1.4 2000/01/23 17:04:11 mycroft Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${S}/arch/sparc64/include machine)
.NOPATH: machine
CLEANFILES+= machine
.endif
