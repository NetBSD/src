#	$NetBSD: Makefile.inc,v 1.2 2002/07/10 13:09:04 scw Exp $

SRCS+=	alloca.S \
	byte_swap_2.S byte_swap_4.S byte_swap_8.S \
	fabs.S flt_rounds.S \
	sigsetjmp.S _setjmp.S

SRCS+=	ieee754_frexp.c ieee754_infinity.c ieee754_isinf.c ieee754_isnan.c \
	ieee754_ldexp.c ieee754_modf.c ieee754_nanf.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
