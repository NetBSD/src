#	$NetBSD: Makefile.inc,v 1.3 2002/10/26 06:44:54 thorpej Exp $

SRCS+=	bswap16.c bswap32.c bswap64.c __sigsetjmp14.S __setjmp14.S _setjmp.S

# Common ieee754 constants and functions
SRCS+=	ieee754_infinity.c ieee754_nanf.c
SRCS+=	ieee754_frexp.c ieee754_ldexp.c
SRCS+=	ieee754_isinf.c
SRCS+=	ieee754_isnan.c
SRCS+=	ieee754_modf.c

# XXX Missing:
# fabs, flt_rounds, fpgetmask, fpgetround, fpgetsticky
# fpsetmask, fpsetround, fpsetsticky
