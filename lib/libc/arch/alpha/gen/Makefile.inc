#	$NetBSD: Makefile.inc,v 1.10 2003/01/21 20:14:07 kleink Exp $

SRCS+=	fabs.S 

# Common ieee754 constants and functions
SRCS+=	ieee754_infinity.c ieee754_nanf.c
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c ieee754_ldexp.c
SRCS+=	ieee754_modf.c

SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
SRCS+=	_setjmp.S
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

LSRCS+=	Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c

SRCS+=	makecontext.c resumecontext.c swapcontext.S _lwp.c

LSRCS+=	Lint_swapcontext.c
DPSRCS+= Lint_swapcontext.c
