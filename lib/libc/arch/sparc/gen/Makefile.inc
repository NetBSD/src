#	$NetBSD: Makefile.inc,v 1.4 1998/09/29 05:55:48 christos Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.c isnan.c ldexp.c modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S
