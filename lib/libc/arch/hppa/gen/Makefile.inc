#	$NetBSD: Makefile.inc,v 1.4 2003/05/12 15:15:13 kleink Exp $

SRCS+=	bswap16.c bswap32.c bswap64.c __sigsetjmp14.S __setjmp14.S _setjmp.S

# Common ieee754 constants and functions
SRCS+=	infinity_ieee754.c nanf_ieee754.c
SRCS+=	frexp_ieee754.c ldexp_ieee754.c
SRCS+=	isinf_ieee754.c
SRCS+=	isnan_ieee754.c
SRCS+=	modf_ieee754.c

# XXX Missing:
# fabs, flt_rounds, fpgetmask, fpgetround, fpgetsticky
# fpsetmask, fpsetround, fpsetsticky
