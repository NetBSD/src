#	$NetBSD: Makefile.inc,v 1.8 2003/10/30 22:46:17 kleink Exp $

SRCS+=	bswap16.c bswap32.c bswap64.c __sigsetjmp14.S __setjmp14.S _setjmp.S

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
SRCS+=	nanf_ieee754.c
SRCS+=	frexp_ieee754.c ldexp_ieee754.c
SRCS+=	isinf_ieee754.c isinfl_dbl_ieee754.c
SRCS+=	isnan_ieee754.c isnanl_dbl_ieee754.c
SRCS+=	modf_ieee754.c

# XXX Missing:
# fabs, flt_rounds, fpgetmask, fpgetround, fpgetsticky
# fpsetmask, fpsetround, fpsetsticky
