# $NetBSD: Makefile.inc,v 1.5 1997/10/22 23:21:46 lukem Exp $

KMINCLUDES=
KMSRCS=
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
