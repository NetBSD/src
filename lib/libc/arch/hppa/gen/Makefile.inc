#	$NetBSD: Makefile.inc,v 1.1 2002/06/06 20:31:21 fredette Exp $

#	$OpenBSD: Makefile.inc,v 1.2 1999/09/14 00:21:15 mickey Exp $

SRCS+=	isnan.c isinf.c infinity.c ldexp.c \
	bswap16.c bswap32.c bswap64.c \
	__sigsetjmp14.S sigsetjmp.S __setjmp14.S _setjmp.S
