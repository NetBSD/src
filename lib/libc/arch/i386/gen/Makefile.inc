#	$NetBSD: Makefile.inc,v 1.5 1997/11/05 23:37:06 cgd Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	_setjmp.S alloca.S fabs.S modf.S setjmp.S sigsetjmp.S \
	flt_rounds.S fpgetmask.S fpgetround.S fpgetsticky.S \
	fpsetmask.S fpsetround.S fpsetsticky.S 

# objects built from C sources
SRCS+=	frexp.c infinity.c isinf.c ldexp.c

# "internal" objects (don't provide part of the user-visible API)
SRCS+=	divsi3.S fixdfsi.S fixunsdfsi.S udivsi3.S

LSRCS+=	Lint__setjmp.c Lint_alloca.c Lint_fabs.c Lint_modf.c \
	Lint_setjmp.c Lint_sigsetjmp.c Lint_flt_rounds.c \
	Lint_fpgetmask.c Lint_fpgetround.c Lint_fpgetsticky.c \
	Lint_fpsetmask.c Lint_fpsetround.c Lint_fpsetsticky.c 
DPSRCS+= Lint__setjmp.c Lint_alloca.c Lint_fabs.c Lint_modf.c \
	Lint_setjmp.c Lint_sigsetjmp.c Lint_flt_rounds.c \
	Lint_fpgetmask.c Lint_fpgetround.c Lint_fpgetsticky.c \
	Lint_fpsetmask.c Lint_fpsetround.c Lint_fpsetsticky.c
