#	$NetBSD: Makefile.inc,v 1.2.2.1 2000/06/23 16:17:16 minoura Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S

SRCS+=	fabs.c flt_rounds.c frexp.c infinity.c \
	isinf.c isnan.c ldexp.c modf.c nanf.c \
	bswap16.c bswap32.c bswap64.c

SRCS+=	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstrSI.S \
	mulsi3.S sdivsi3.S udivsi3.S

KMSRCS+= ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstrSI.S \
	mulsi3.S sdivsi3.S udivsi3.S
