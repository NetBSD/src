# $NetBSD: Makefile.inc,v 1.6 1999/01/15 13:31:16 bouyer Exp $

SRCS+=	alloca.S byte_swap_2.S byte_swap_4.S bswap64.c divsi3.S \
	fabs.c flt_rounds.c fpgetround.c fpsetround.c \
	fpgetmask.S fpsetmask.S fpgetsticky.S fpsetsticky.S \
	infinity.c isinf.c frexp.c ldexp.c modf.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c
