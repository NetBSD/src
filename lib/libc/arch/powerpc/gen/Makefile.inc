#	$NetBSD: Makefile.inc,v 1.7.6.1 2001/10/08 20:17:53 nathanw Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S
SRCS+=	isinf.c isnan.c ldexp.c fabs.c flt_rounds.c infinity.c nanf.c
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	frexp.c modf.c
SRCS+=	syncicache.c
SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c
SRCS+=	fpgetsticky.c fpsetsticky.c

KMSRCS+=syncicache.c
