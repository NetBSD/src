#	$NetBSD: Makefile.inc,v 1.14 2003/05/12 15:15:13 kleink Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	alloca.S byte_swap_2.S byte_swap_4.S fabs.S modf.S \
	flt_rounds.S fpgetmask.S fpgetround.S fpgetsticky.S \
	fpsetmask.S fpsetround.S fpsetsticky.S 

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	resumecontext.S swapcontext.S

# objects built from C sources
SRCS+=	bswap64.c ldexp.c _lwp.c makecontext.c

# Common ieee754 constants and functions
SRCS+=	infinity_ieee754.c nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isnan_ieee754.c


# "internal" objects (don't provide part of the user-visible API)
SRCS+=	divsi3.S fixdfsi.S fixunsdfsi.S udivsi3.S

LSRCS+=	Lint__setjmp.c Lint_alloca.c Lint_bswap16.c Lint_bswap32.c \
	Lint_fabs.c Lint_modf.c \
	Lint___setjmp14.c Lint___sigsetjmp14.c Lint_flt_rounds.c \
	Lint_fpgetmask.c Lint_fpgetround.c Lint_fpgetsticky.c \
	Lint_fpsetmask.c Lint_fpsetround.c Lint_fpsetsticky.c \
	Lint_resumecontext.c Lint_swapcontext.c
DPSRCS+= Lint__setjmp.c Lint_alloca.c Lint_bswap16.c Lint_bswap32.c \
	Lint_fabs.c Lint_modf.c \
	Lint___setjmp14.c Lint___sigsetjmp14.c Lint_flt_rounds.c \
	Lint_fpgetmask.c Lint_fpgetround.c Lint_fpgetsticky.c \
	Lint_fpsetmask.c Lint_fpsetround.c Lint_fpsetsticky.c \
	Lint_resumecontext.c Lint_swapcontext.c
