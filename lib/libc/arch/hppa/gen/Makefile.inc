#	$NetBSD: Makefile.inc,v 1.7 2003/10/25 22:31:20 kleink Exp $

SRCS+=	bswap16.c bswap32.c bswap64.c __sigsetjmp14.S __setjmp14.S _setjmp.S

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c
SRCS+=	nanf_ieee754.c
SRCS+=	frexp_ieee754.c ldexp_ieee754.c
SRCS+=	isinf_ieee754.c
SRCS+=	isnan_ieee754.c
SRCS+=	modf_ieee754.c

SRCS+=	infinityl.c isinfl.c isnanl.c

# XXX Missing:
# fabs, flt_rounds, fpgetmask, fpgetround, fpgetsticky
# fpsetmask, fpsetround, fpsetsticky
