# $NetBSD: Makefile.inc,v 1.4 1997/10/13 21:26:58 mark Exp $

KMINCLUDES=
KMSRCS=
CFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
