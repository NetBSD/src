#	$NetBSD: Makefile.inc,v 1.11 2003/05/12 15:15:11 kleink Exp $

SRCS+=	fabs.S 

# Common ieee754 constants and functions
SRCS+=	infinity_ieee754.c nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isnan_ieee754.c ldexp_ieee754.c
SRCS+=	modf_ieee754.c

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
