#	$NetBSD: Makefile.inc,v 1.4.6.4 2002/03/22 20:41:55 nathanw Exp $

SRCS+=	fabs.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

# Common ieee754 constants and functions
SRCS+=	ieee754_infinity.c ieee754_nanf.c
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c ieee754_ldexp.c

SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c _lwp.c swapcontext.S

SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S

LSRCS+=	Lint_swapcontext.c
DPSRCS+=Lint_swapcontext.c
