#	$NetBSD: Makefile.inc,v 1.3 1999/01/15 13:31:20 bouyer Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.c isnan.c ldexp.c modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S

