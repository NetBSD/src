#	$NetBSD: Makefile.inc,v 1.3 2000/06/06 17:28:40 tsubai Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S

SRCS+=	fabs.c flt_rounds.c frexp.c infinity.c \
	isinf.c isnan.c ldexp.c modf.c nanf.c \
	bswap16.c bswap32.c bswap64.c

SRCS+=	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S mulsi3.S \
	sdivsi3.S udivsi3.S

KMSRCS+= ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S mulsi3.S \
	sdivsi3.S udivsi3.S
