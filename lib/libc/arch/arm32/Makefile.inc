# $NetBSD: Makefile.inc,v 1.6.2.1 1999/09/22 06:08:27 cgd Exp $

KMINCLUDES=
KMSRCS=arch/arm32/gen/divsi3.S
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
