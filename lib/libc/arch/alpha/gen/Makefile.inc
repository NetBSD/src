#	$NetBSD: Makefile.inc,v 1.4 1998/09/29 03:01:37 thorpej Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.c ldexp.c modf.c
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
SRCS+=	_setjmp.S
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
