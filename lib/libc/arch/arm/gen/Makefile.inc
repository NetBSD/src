# $NetBSD: Makefile.inc,v 1.2 2002/02/19 13:08:17 simonb Exp $

SRCS+=	alloca.S byte_swap_2.S byte_swap_4.S bswap64.c divsi3.S \
	fabs.c flt_rounds.c \
	infinity.c

# Common ieee754 constants and functions
SRCS+=	ieee754_nanf.c	# infinity is ``different'' on arm, use local version
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c ieee754_ldexp.c
SRCS+=	ieee754_modf.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c
