# $NetBSD: Makefile.inc,v 1.9 2000/12/13 07:05:56 enami Exp $

KMINCLUDES=
KMSRCS=arch/arm32/gen/divsi3.S __syscall.S
SRCS+=__syscall.S
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
