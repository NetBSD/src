# $NetBSD: Makefile.inc,v 1.6 1999/01/15 13:31:16 bouyer Exp $

KMINCLUDES=
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
