#	$NetBSD: Makefile.inc,v 1.13 2002/02/19 13:08:23 simonb Exp $

SRCS+=	fabs.S ldexp.S modf.S

# Common ieee754 constants and functions
SRCS+=	ieee754_infinity.c ieee754_nanf.c
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c

SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

# mips abi builtin extensions (used by GCC for lexical-closure trampoline)
SRCS+=	cacheflush.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c
