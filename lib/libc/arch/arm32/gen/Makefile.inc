# $NetBSD: Makefile.inc,v 1.7 1999/08/29 18:08:29 mycroft Exp $

SRCS+=	alloca.S byte_swap_2.S byte_swap_4.S bswap64.c divsi3.S \
	fabs.c flt_rounds.c fpgetround.c fpsetround.c \
	fpgetmask.S fpsetmask.S fpgetsticky.S fpsetsticky.S \
	infinity.c isinf.c isnan.c frexp.c ldexp.c modf.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c
