#	$NetBSD: Makefile.inc,v 1.9 1999/01/22 14:22:36 jonb Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=alloca.S fabs.S frexp.S ldexp.S modf.S

ASSRCS+=__setjmp14.S
ASSRCS+=_setjmp.S
ASSRCS+=__sigsetjmp14.S

# objects built from assembler sources (without lint stubs)
SRCS+=	setjmp.S sigsetjmp.S

# objects built from C sources
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c infinity.c isinf.c bswap64.c
