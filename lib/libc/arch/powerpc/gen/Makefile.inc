#	$NetBSD: Makefile.inc,v 1.5 1999/04/17 21:16:45 ws Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S
SRCS+=	isinf.c isnan.c ldexp.c fabs.S flt_rounds.c infinity.c
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	frexp.c modf.c
SRCS+=	syncicache.c

KMSRCS+=syncicache.c
