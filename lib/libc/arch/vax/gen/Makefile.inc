#	$NetBSD: Makefile.inc,v 1.9 2003/05/17 15:05:55 thorpej Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.S \
	fabs.S frexp.c \
	infinity.c isinf.c isinfl.c isnan.c isnanl.c \
	ldexp.S \
	makecontext.c modf.S \
	resumecontext.c \
	swapcontext.S \
	udiv.S urem.S \
	__setjmp14.S __sigsetjmp14.S _setjmp.S 

.if ${OBJECT_FMT} != "ELF"
SRCS+=	setjmp.S sigsetjmp.S
.endif

LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
