# $NetBSD: Makefile.inc,v 1.8 2000/12/12 22:38:42 simonb Exp $

KMINCLUDES=
KMSRCS=arch/arm32/gen/divsi3.S __syscall.S
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
