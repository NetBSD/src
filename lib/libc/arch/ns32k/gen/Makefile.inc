#	$NetBSD: Makefile.inc,v 1.13 1999/12/23 10:15:07 kleink Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=alloca.S fabs.S modf.S

ASSRCS+=__setjmp14.S
ASSRCS+=_setjmp.S
ASSRCS+=__sigsetjmp14.S

# objects built from assembler sources (without lint stubs)
SRCS+=	setjmp.S sigsetjmp.S

# objects built from C sources
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c infinity.c frexp.c isinf.c isnan.c ldexp.c \
	nanf.c \
	bswap64.c
