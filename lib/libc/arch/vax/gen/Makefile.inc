#	$NetBSD: Makefile.inc,v 1.5.12.2 2002/11/11 22:21:57 nathanw Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.S \
	fabs.S frexp.c infinity.c isinf.c isnan.c ldexp.S modf.S \
	udiv.S urem.S \
	__setjmp14.S __sigsetjmp14.S _setjmp.S 

.if ${OBJECT_FMT} != "ELF"
SRCS+=	setjmp.S sigsetjmp.S
.endif

LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
