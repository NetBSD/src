#	$NetBSD: Makefile.inc,v 1.12.6.1 2001/11/21 08:52:41 wdk Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.c isnan.c ldexp.S modf.S nanf.c
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

SRCS+=	makecontext.c resumecontext.c swapcontext.S

# mips abi builtin extensions (used by GCC for lexical-closure trampoline)
SRCS+=	cacheflush.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c

LSRCS+=	Lint_swapcontext.c
DPSRCS+= Lint_swapcontext.c
