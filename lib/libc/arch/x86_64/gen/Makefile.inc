#	$NetBSD: Makefile.inc,v 1.1 2001/06/19 00:25:02 fvdl Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	alloca.S bswap64.S byte_swap_2.S byte_swap_4.S fabs.S modf.S \
	flt_rounds.S fpgetmask.S fpgetround.S fpgetsticky.S \
	fpsetmask.S fpsetround.S fpsetsticky.S 

SRCS+=	__setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	__sigsetjmp14.S

# objects built from C sources
SRCS+= frexp.c infinity.c isinf.c isnan.c ldexp.c nanf.c

LSRCS+=	Lint__setjmp.c Lint_alloca.c Lint_bswap16.c Lint_bswap32.c \
	Lint_bswap64.c Lint_fabs.c Lint_modf.c \
	Lint___setjmp14.c Lint___sigsetjmp14.c Lint_flt_rounds.c \
	Lint_fpgetmask.c Lint_fpgetround.c Lint_fpgetsticky.c \
	Lint_fpsetmask.c Lint_fpsetround.c Lint_fpsetsticky.c 
DPSRCS+= Lint__setjmp.c Lint_alloca.c Lint_bswap16.c Lint_bswap32.c \
	Lint_bswap64.c Lint_fabs.c Lint_modf.c \
	Lint___setjmp14.c Lint___sigsetjmp14.c Lint_flt_rounds.c \
	Lint_fpgetmask.c Lint_fpgetround.c Lint_fpgetsticky.c \
	Lint_fpsetmask.c Lint_fpsetround.c Lint_fpsetsticky.c
