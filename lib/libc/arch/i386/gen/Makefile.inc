#	$NetBSD: Makefile.inc,v 1.3 1995/04/07 14:57:16 jtc Exp $

SRCS+=	_setjmp.S alloca.S fabs.S frexp.c infinity.c isinf.c ldexp.c modf.S \
	setjmp.S sigsetjmp.S
SRCS+=	flt_rounds.S fpgetmask.S fpgetround.S fpgetsticky.S
SRCS+=	divsi3.S fixdfsi.S fixunsdfsi.S udivsi3.S
