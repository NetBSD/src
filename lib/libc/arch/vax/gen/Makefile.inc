#	$NetBSD: Makefile.inc,v 1.4 1999/01/15 13:31:20 bouyer Exp $

SRCS+=	_setjmp.S byte_swap_2.S byte_swap_4.S bswap64.S fabs.S frexp.c \
	infinity.c isinf.c ldexp.S modf.S setjmp.S udiv.S urem.S alloca.S \
	sigsetjmp.S

LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
