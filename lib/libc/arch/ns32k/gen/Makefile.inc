#	$NetBSD: Makefile.inc,v 1.14 2002/02/19 13:08:24 simonb Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=alloca.S fabs.S modf.S

ASSRCS+=__setjmp14.S
ASSRCS+=_setjmp.S
ASSRCS+=__sigsetjmp14.S

# objects built from assembler sources (without lint stubs)
SRCS+=	setjmp.S sigsetjmp.S

# objects built from C sources
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c \
	bswap64.c

# Common ieee754 constants and functions
SRCS+=	ieee754_nanf.c	ieee754_infinity.c
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c ieee754_ldexp.c
