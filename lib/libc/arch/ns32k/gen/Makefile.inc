#	$NetBSD: Makefile.inc,v 1.21 2004/01/15 19:43:45 kleink Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=alloca.S fabs.S modf.S

ASSRCS+=__setjmp14.S
ASSRCS+=_setjmp.S
ASSRCS+=__sigsetjmp14.S

ASSRCS+=resumecontext.S swapcontext.S

# objects built from assembler sources (without lint stubs)
SRCS+=	setjmp.S sigsetjmp.S

# objects built from C sources
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c \
	bswap64.c

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
SRCS+=	nanf_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isinfl_dbl_ieee754.c isnan_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c
SRCS+=	isnanl_dbl_ieee754.c ldexp_ieee754.c
SRCS+=	signbitf_ieee754.c signbitd_ieee754.c

SRCS+=	makecontext.c

SRCS.ns32k.gen=	Lint_resumecontext.c Lint_swapcontext.c
LSRCS+=		${SRCS.ns32k.gen}
DPSRCS+=	${SRCS.ns32k.gen}
CLEANFILES+=	${SRCS.ns32k.gen}
