#	$NetBSD: Makefile.inc,v 1.1 2002/07/05 13:31:29 scw Exp $

SRCS+=	alloca.S \
	byte_swap_2.S byte_swap_4.S byte_swap_8.S \
	fabs.S \
	sigsetjmp.S _setjmp.S

LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
