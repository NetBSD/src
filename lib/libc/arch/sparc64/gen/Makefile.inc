#	$NetBSD: Makefile.inc,v 1.4.6.1 2001/11/17 12:25:30 martin Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.c isnan.c ldexp.c modf.S nanf.c
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c swapcontext.S

SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S

LSRCS+=	Lint_swapcontext.c
DPSRCS+=Lint_swapcontext.c
