#	$NetBSD: Makefile.inc,v 1.5 1999/01/15 13:31:15 bouyer Exp $

SRCS+=	fabs.S frexp.c infinity.c isinf.c ldexp.c modf.c
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
SRCS+=	_setjmp.S
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c
