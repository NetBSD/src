#	$NetBSD: Makefile.inc,v 1.7.6.4 2002/03/22 20:41:53 nathanw Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	fabs.c flt_rounds.c
SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c
SRCS+=	fpgetsticky.c fpsetsticky.c
SRCS+=	syncicache.c
SRCS+= _lwp.c makecontext.c resumecontext.c swapcontext.S

# Common ieee754 constants and functions
SRCS+=	ieee754_infinity.c ieee754_nanf.c
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c ieee754_ldexp.c
SRCS+=	ieee754_modf.c

LSRCS+=	Lint_swapcontext.c
DPSRCS+=Lint_swapcontext.c
