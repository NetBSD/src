#	$NetBSD: Makefile.inc,v 1.5 1997/10/17 06:29:02 jonathan Exp $

SRCS+=	_setjmp.S fabs.S frexp.c infinity.c isinf.S ldexp.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
SRCS+=	setjmp.S sigsetjmp.S

# mips abi builtin extensions (used by GCC for lexical-closure trampoline)
SRCS+=	cacheflush.c
