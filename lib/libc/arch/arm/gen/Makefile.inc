# $NetBSD: Makefile.inc,v 1.1.2.2 2001/11/16 03:44:25 thorpej Exp $

SRCS+=	alloca.S byte_swap_2.S byte_swap_4.S bswap64.c divsi3.S \
	fabs.c flt_rounds.c \
	infinity.c isinf.c isnan.c frexp.c ldexp.c modf.c nanf.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c swapcontext.S

SRCS+=	_lwp.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c

LSRCS+=	Lint_swapcontext.c
DPSRCS+= Lint_swapcontext.c
