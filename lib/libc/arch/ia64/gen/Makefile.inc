#	$NetBSD: Makefile.inc,v 1.4.42.1 2013/01/23 00:05:22 yamt Exp $

SRCS+=	bswap16.c bswap32.c bswap64.c

SRCS+= flt_rounds.c fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c
SRCS+=	isinff_ieee754.c isinfd_ieee754.c
SRCS+=	isnanf_ieee754.c isnand_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c

