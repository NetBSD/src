# $NetBSD: Makefile.inc,v 1.3 1996/09/09 22:03:24 mark Exp $

SRCS+=	_setjmp.S alloca.S \
	fabs.c flt_rounds.c fpgetround.c fpsetround.c \
	fpgetmask.S fpsetmask.S fpgetsticky.S fpsetsticky.S \
	infinity.c isinf.c frexp.c ldexp.c modf.c \
	setjmp.S sigsetjmp.S
