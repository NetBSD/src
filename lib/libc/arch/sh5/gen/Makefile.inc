#	$NetBSD: Makefile.inc,v 1.5 2003/01/19 23:05:01 scw Exp $

SRCS+=	__setjmp14.S __sigsetjmp14.S _setjmp.S \
	alloca.S \
	byte_swap_2.S byte_swap_4.S byte_swap_8.S \
	fabs.S flt_rounds.S \

SRCS+=	ieee754_frexp.c ieee754_infinity.c ieee754_isinf.c ieee754_isnan.c \
	ieee754_ldexp.c ieee754_modf.c ieee754_nanf.c

SRCS+=	fpgetmask.S fpgetround.S fpgetsticky.S \
	fpsetmask.S fpsetround.S fpsetsticky.S

SRCS+=	makecontext.c resumecontext.c swapcontext.S _lwp.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
