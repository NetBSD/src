# $NetBSD: Makefile.inc,v 1.10 2000/12/13 07:44:34 matt Exp $

KMINCLUDES=
KMSRCS=arch/arm32/gen/divsi3.S
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
