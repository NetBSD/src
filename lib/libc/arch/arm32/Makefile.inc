# $NetBSD: Makefile.inc,v 1.7 1999/09/21 09:43:38 is Exp $

KMINCLUDES=
KMSRCS=arch/arm32/gen/divsi3.S
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
