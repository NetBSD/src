# $NetBSD: Makefile.inc,v 1.4 1997/10/06 00:12:24 mark Exp $

SRCS+=	_setjmp.S alloca.S divsi3.S \
	fabs.c flt_rounds.c fpgetround.c fpsetround.c \
	fpgetmask.S fpsetmask.S fpgetsticky.S fpsetsticky.S \
	infinity.c isinf.c frexp.c ldexp.c modf.c \
	setjmp.S sigsetjmp.S
