# $NetBSD: Makefile.inc,v 1.5 1998/09/29 20:23:25 thorpej Exp $

SRCS+=	alloca.S divsi3.S \
	fabs.c flt_rounds.c fpgetround.c fpsetround.c \
	fpgetmask.S fpsetmask.S fpgetsticky.S fpsetsticky.S \
	infinity.c isinf.c frexp.c ldexp.c modf.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
