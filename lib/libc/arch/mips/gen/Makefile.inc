#	$NetBSD: Makefile.inc,v 1.6 1998/09/27 22:59:39 jonathan Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.S ldexp.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

# mips abi builtin extensions (used by GCC for lexical-closure trampoline)
SRCS+=	cacheflush.c
