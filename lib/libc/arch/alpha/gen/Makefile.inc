#	$NetBSD: Makefile.inc,v 1.7.6.1 2001/11/17 03:19:29 nathanw Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.c isnan.c ldexp.c modf.c nanf.c
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
SRCS+=	_setjmp.S
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

SRCS+=	makecontext.c resumecontext.c swapcontext.S _lwp.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c
