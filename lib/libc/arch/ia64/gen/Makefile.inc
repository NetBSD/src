#	$NetBSD: Makefile.inc,v 1.7 2016/06/30 09:14:30 mrg Exp $

SRCS+=	_lwp.c
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	setjmp.S _setjmp.S sigsetjmp.S
SRCS+=	flt_rounds.c fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c
SRCS+=	isinff_ieee754.c isinfd_ieee754.c
SRCS+=	isnanf_ieee754.c isnand_ieee754.c
SRCS+=	signbitf_ieee754.c signbitd_ieee754.c

