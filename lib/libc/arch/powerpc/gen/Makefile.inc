#	$NetBSD: Makefile.inc,v 1.7.6.2 2001/11/08 05:38:11 briggs Exp $

SRCS+=	_lwp.c _setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S
SRCS+=	makecontext.c resumecontext.c swapcontext.S
SRCS+=	isinf.c isnan.c ldexp.c fabs.c flt_rounds.c infinity.c nanf.c
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	frexp.c modf.c
SRCS+=	syncicache.c
SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c
SRCS+=	fpgetsticky.c fpsetsticky.c

KMSRCS+=syncicache.c

LSRCS+=	Lint_swapcontext.c
DPSRCS+=Lint_swapcontext.c
