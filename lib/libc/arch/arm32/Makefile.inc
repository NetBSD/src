# $NetBSD: Makefile.inc,v 1.4.2.1 1997/11/09 00:59:49 lukem Exp $

KMINCLUDES=
KMSRCS=
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
